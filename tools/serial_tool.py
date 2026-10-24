import argparse
import ctypes
import datetime as datetime_module
import os
import select
import serial
import shutil
import sys
import threading
from typing import Callable, Dict, List, Optional, Tuple


PACKET_PREFIX = b"\x1b\x1e"
PACKET_TYPES = {b"Console", b"Struct", b"Log"}
LOG_LEVELS = {"TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"}


class TestStruct(ctypes.Structure):
    """STM32 Struct 协议联调用结构体。"""

    _pack_ = 1

    _fields_ = [
        ("first", ctypes.c_uint16),
        ("second", ctypes.c_uint8),
        ("third", ctypes.c_uint32),
        ("fourth", ctypes.c_double),
    ]


def escape_text(value: str | bytes, encoding: str = "utf-8") -> bytes:
    """转义文本字段中的反斜杠、方括号和 NUL。"""
    data = value.encode(encoding) if isinstance(value, str) else bytes(value)
    return (data.replace(b"\\", b"\\\\")
                .replace(b"\x00", b"\\0")
                .replace(b"[", b"\\[")
                .replace(b"]", b"\\]"))


def unescape_text(value: bytes, encoding: str = "utf-8") -> str:
    """还原 escape_text 生成的文本字段。"""
    output = bytearray()
    index = 0
    while index < len(value):
        if value[index] == ord("\\") and index + 1 < len(value):
            escaped = value[index + 1]
            output.append(0 if escaped == ord("0") else escaped)
            index += 2
        else:
            output.append(value[index])
            index += 1
    return bytes(output).decode(encoding, errors="replace")


def read_bracket_field(data: bytes, start: int) -> Tuple[Optional[bytes], int]:
    """读取一个 [文本字段]，字段内部允许反斜杠转义方括号。"""
    if start >= len(data) or data[start] != ord("["):
        return None, start
    field = bytearray()
    index = start + 1
    escaped = False
    while index < len(data):
        byte = data[index]
        if escaped:
            field.extend((ord("\\"), byte))
            escaped = False
        elif byte == ord("\\"):
            escaped = True
        elif byte == ord("]"):
            return bytes(field), index + 1
        else:
            field.append(byte)
        index += 1
    return None, start


def read_packet_header(data: bytes) -> Tuple[Optional[bytes], int]:
    """解析 ESC RS 和 [Console]/[Struct]/[Log]。"""
    if not data.startswith(PACKET_PREFIX):
        return None, 0
    packet_type, offset = read_bracket_field(data, len(PACKET_PREFIX))
    if packet_type not in PACKET_TYPES:
        return None, 0
    return packet_type, offset


class LogHandler:
    def __init__(self, file_paths: Optional[Dict[str, str]] = None,
                 output: Optional[Callable[[str], None]] = None) -> None:
        self.file_paths = file_paths or {}
        self.output = output or print

    def handle(self, level: str, console: int, file_tag: str,
               message: str) -> None:
        timestamp = datetime_module.datetime.now().strftime("%H:%M:%S.%f")[:-3]
        tag = f" [{file_tag}]" if file_tag else ""
        line = f"{timestamp} [{level}]{tag} {message}"
        if console:
            self.output(line)
        path = self.file_paths.get(file_tag)
        if path:
            directory = os.path.dirname(os.path.abspath(path))
            os.makedirs(directory, exist_ok=True)
            with open(path, "a", encoding="utf-8", newline="") as stream:
                stream.write(line + "\n")


class PacketDecoder:
    """增量解析普通文本和 ESC RS 数据包。"""

    def __init__(self, encoding: str, namespace: Dict[str, object],
                 on_text: Callable[[bytes], None],
                 on_message: Callable[[str], None],
                 log_handler: LogHandler,
                 on_struct: Optional[Callable[[str, str, bytes], None]] = None) -> None:
        self.encoding = encoding
        self.namespace = namespace
        self.on_text = on_text
        self.on_message = on_message
        self.log_handler = log_handler
        self.on_struct = on_struct
        self.buffer = bytearray()

    def feed(self, data: bytes) -> None:
        self.buffer.extend(data)
        self._drain()

    def _drain(self) -> None:
        while self.buffer:
            prefix_index = self.buffer.find(PACKET_PREFIX)
            if prefix_index < 0:
                # 保留末尾 ESC，等待下一次数据判断它是否是包头。
                keep = 1 if self.buffer[-1] == PACKET_PREFIX[0] else 0
                if len(self.buffer) > keep:
                    self.on_text(bytes(self.buffer[:len(self.buffer) - keep]))
                    del self.buffer[:len(self.buffer) - keep]
                return
            if prefix_index:
                self.on_text(bytes(self.buffer[:prefix_index]))
                del self.buffer[:prefix_index]
            consumed = self._decode_packet()
            if consumed is None:
                return
            if consumed == 0:
                self.on_text(bytes(self.buffer[:1]))
                del self.buffer[:1]
            else:
                del self.buffer[:consumed]

    def _decode_packet(self) -> Optional[int]:
        data = bytes(self.buffer)
        packet_type, offset = read_packet_header(data)
        if packet_type is None:
            if data.startswith(PACKET_PREFIX) and b"]" not in data[len(PACKET_PREFIX):]:
                return None
            return 0
        if packet_type == b"Console":
            return self._decode_console(data, offset)
        if packet_type == b"Log":
            return self._decode_log(data, offset)
        return self._decode_struct(data, offset)

    def _decode_console(self, data: bytes, offset: int) -> Optional[int]:
        payload, end = read_bracket_field(data, offset)
        if payload is None:
            return None
        self.on_text(unescape_text(payload, self.encoding).encode(self.encoding))
        return end

    def _decode_log(self, data: bytes, offset: int) -> Optional[int]:
        fields: List[bytes] = []
        for _ in range(3):
            field, offset = read_bracket_field(data, offset)
            if field is None:
                return None
            fields.append(field)
        level = fields[0].decode("ascii", errors="replace")
        try:
            console = int(fields[1])
        except ValueError:
            self.on_message("协议错误：Log console 字段无效")
            return self._discard_to_next_packet(data)
        file_tag = unescape_text(fields[2], self.encoding)
        payload, end = read_bracket_field(data, offset)
        if payload is None:
            return None
        if level not in LOG_LEVELS or console not in (0, 1):
            self.on_message(f"协议错误：Log level 或 console 字段无效，接收到的原始数据：{data}")
        else:
            self.log_handler.handle(level, console, file_tag,
                                    unescape_text(payload, self.encoding))
        return end

    def _decode_struct(self, data: bytes, offset: int) -> Optional[int]:
        """解析 Struct 包；布局与对齐完全由已注册的 ctypes.Structure 决定。"""
        fields: List[bytes] = []
        for _ in range(2):
            field, offset = read_bracket_field(data, offset)
            if field is None:
                return None
            fields.append(field)
        class_name = fields[0].decode("ascii", errors="replace")
        variable_name = fields[1].decode("ascii", errors="replace")
        struct_type = self.namespace.get(class_name)
        if not isinstance(struct_type, type) or not issubclass(struct_type, ctypes.Structure):
            self.on_message(f"协议错误：未注册 ctypes.Structure: {class_name}")
            return self._discard_to_next_packet(data)
        try:
            expected_size = ctypes.sizeof(struct_type)
        except (TypeError, ValueError) as error:
            self.on_message(f"协议错误：无法计算 {class_name} 大小: {error}")
            return self._discard_to_next_packet(data)
        if len(data) < offset + expected_size + 2:
            return None
        if data[offset] != ord("["):
            self.on_message("协议错误：Struct payload 缺少左方括号")
            return self._discard_to_next_packet(data)
        payload_start = offset + 1
        payload_end = payload_start + expected_size
        if data[payload_end] != ord("]"):
            self.on_message(f"协议错误：Struct payload 尾部无效，传输的原始数据：{data}")
            return self._discard_to_next_packet(data)
        payload_bytes = data[payload_start:payload_end]
        self.namespace[variable_name] = struct_type.from_buffer_copy(payload_bytes)
        if self.on_struct:
            self.on_struct(class_name, variable_name, payload_bytes)
        return payload_end + 1

    @staticmethod
    def _discard_to_next_packet(data: bytes) -> Optional[int]:
        """丢弃损坏包；无长度信息时等待下一个包头重新同步。"""
        next_packet = data.find(PACKET_PREFIX, len(PACKET_PREFIX))
        return None if next_packet < 0 else next_packet


class EnhancedSerialTool:
    def __init__(self, port: str, baudrate: int = 9600,
                 encoding: str = "utf-8",
                 log_paths: Optional[Dict[str, str]] = None,
                 show_struct: bool = False) -> None:
        self.port = port
        self.baudrate = baudrate
        self.encoding = encoding
        self.serial_conn = None
        self.stop_event = threading.Event()
        self.input_buffer = ""
        self.raw_mode = False
        self.received_bytes = 0
        self.sent_bytes = 0
        self.output_lines: List[str] = []
        self.output_lock = threading.Lock()
        self.screen_enabled = False
        self.last_output_was_received = False
        self.show_struct = show_struct
        self.decoder = PacketDecoder(
            encoding, globals(), self._handle_text_bytes, self._append_output,
            LogHandler(log_paths, self._append_output),
            self._handle_struct if show_struct else None)

    def _handle_struct(self, class_name: str, variable_name: str,
                       payload: bytes) -> None:
        display = " ".join(f"{byte:02X}" for byte in payload)
        self._append_output(
            f"[STRUCT] {class_name} -> {variable_name}: {display}")

    def _append_output(self, text: str, start_new_line: bool = True) -> None:
        if not text:
            return
        text = str(text).replace("\r\n", "\n").replace("\r", "\n")
        with self.output_lock:
            if not self.output_lines:
                self.output_lines.append("")
            elif self.output_lines[-1] and (start_new_line or not self.last_output_was_received):
                self.output_lines.append("")
            parts = text.split("\n")
            self.output_lines[-1] += parts[0]
            self.output_lines.extend(parts[1:])
            self.last_output_was_received = not start_new_line
            self.output_lines = self.output_lines[-2000:]
        self._render_screen()

    def _handle_text_bytes(self, data: bytes) -> None:
        if self.raw_mode:
            self._append_output("[RECV-RAW] " + " ".join(f"{byte:02X}" for byte in data))
        else:
            self._append_output(data.decode(self.encoding, errors="replace"), False)

    def _start_screen(self) -> None:
        self.screen_enabled = True
        sys.stdout.write("\x1b[?25l")
        self._render_screen()

    def _stop_screen(self) -> None:
        if self.screen_enabled:
            sys.stdout.write("\x1b[2J\x1b[H\x1b[?25h")
            sys.stdout.flush()
            self.screen_enabled = False

    def _render_screen(self) -> None:
        if not self.screen_enabled:
            return
        columns, rows = shutil.get_terminal_size((80, 24))
        output_height = max(1, rows - 3)
        with self.output_lock:
            lines = [line[:columns] for line in self.output_lines[-output_height:]]
            lines += [""] * (output_height - len(lines))
        screen = ["\x1b[2J\x1b[H"]
        screen.extend(f"{line}\x1b[K\n" for line in lines)
        screen.append(f"\x1b[1;33m{'-' * columns}\x1b[0m\n")
        input_line = ("输入> " + self.input_buffer)[:columns]
        screen.append(f"{input_line}\x1b[K")
        screen.append(f"\x1b[{output_height + 2};{min(len(input_line), columns) + 1}H")
        sys.stdout.write("".join(screen))
        sys.stdout.flush()

    def connect(self) -> bool:
        try:
            self.serial_conn = serial.Serial(
                port=self.port, baudrate=self.baudrate,
                bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE, timeout=0.1, write_timeout=1)
            if self.serial_conn.is_open:
                self._append_output(f"成功连接到串口: {self.port}")
                self._append_output(f"波特率: {self.baudrate}, 编码: {self.encoding}")
                self._append_output("开始读取数据。输入 exit 退出，raw 切换原始显示模式。")
                return True
        except Exception as error:
            self._append_output(f"连接错误: {error}")
        return False

    def process_received_data(self) -> None:
        while not self.stop_event.is_set():
            try:
                if self.serial_conn and self.serial_conn.is_open:
                    available = self.serial_conn.in_waiting
                    if available:
                        data = self.serial_conn.read(available)
                        self.received_bytes += len(data)
                        self.decoder.feed(data)
                self.stop_event.wait(0.01)
            except Exception as error:
                if not self.stop_event.is_set():
                    self._append_output(f"读取错误: {error}")

    def handle_user_input(self) -> None:
        while not self.stop_event.is_set():
            try:
                if sys.platform == "win32":
                    import msvcrt
                    if msvcrt.kbhit():
                        self._process_input_char(msvcrt.getwch())
                else:
                    ready, _, _ = select.select([sys.stdin], [], [], 0.1)
                    if ready:
                        line = sys.stdin.readline().rstrip("\n\r")
                        if line:
                            self._process_command(line)
                            self._render_screen()
                self.stop_event.wait(0.01)
            except KeyboardInterrupt:
                self._append_output("用户中断程序")
                self.stop_event.set()
            except Exception as error:
                if not self.stop_event.is_set():
                    self._append_output(f"输入错误: {error}")

    def _process_input_char(self, char: str) -> None:
        if char in ("\r", "\n"):
            if self.input_buffer:
                self._process_command(self.input_buffer)
                self.input_buffer = ""
            self._render_screen()
        elif char in ("\x08", "\x7f"):
            self.input_buffer = self.input_buffer[:-1]
            self._render_screen()
        elif char == "\x03":
            self._append_output("用户中断程序")
            self.stop_event.set()
        else:
            self.input_buffer += char
            self._render_screen()

    def _process_command(self, command: str) -> None:
        lowered = command.lower()
        if lowered == "exit":
            self._append_output("正在退出程序...")
            self.stop_event.set()
        elif lowered == "raw":
            self.raw_mode = not self.raw_mode
            self._append_output("切换到" + ("原始模式" if self.raw_mode else "文本模式"))
        elif lowered == "help":
            self._show_help()
        elif lowered == "clear":
            with self.output_lock:
                self.output_lines.clear()
                self.last_output_was_received = False
            self._render_screen()
        elif lowered == "stats":
            self._append_output("统计信息:")
            self._append_output(f"接收字节数: {self.received_bytes}")
            self._append_output(f"发送字节数: {self.sent_bytes}")
        elif lowered.startswith("send "):
            self._send_hex_data(command[5:].strip())
        else:
            self._send_data_to_serial(command)

    def _send_data_to_serial(self, data: str) -> bool:
        try:
            if not self.serial_conn or not self.serial_conn.is_open:
                self._append_output("串口未连接，无法发送数据")
                return False
            packet = data.encode(self.encoding) + b"\n\x00"
            sent = self.serial_conn.write(packet)
            self.serial_conn.flush()
            self.sent_bytes += sent
            return True
        except Exception as error:
            self._append_output(f"发送错误: {error}")
            return False

    def _send_hex_data(self, hex_data: str) -> bool:
        try:
            data = bytes.fromhex(hex_data.replace("0x", "").replace(",", ""))
            if not self.serial_conn or not self.serial_conn.is_open:
                self._append_output("串口未连接，无法发送数据")
                return False
            sent = self.serial_conn.write(data)
            self.serial_conn.flush()
            self.sent_bytes += sent
            display = " ".join(f"{byte:02X}" for byte in data)
            self._append_output(f"[SENT-HEX] {display} (共 {sent} 字节)")
            return True
        except ValueError:
            self._append_output("无效的十六进制数据")
        except Exception as error:
            self._append_output(f"发送错误: {error}")
        return False

    def _show_help(self) -> None:
        self._append_output("""命令帮助:
exit      - 退出程序
raw       - 切换原始显示模式（只影响普通文本）
--show-struct - 在控制台显示已接收的 Struct 原始字节
clear     - 清空输出区
stats     - 显示统计信息
help      - 显示此帮助信息
send <hex>- 发送十六进制数据""")

    def run(self) -> None:
        if not self.connect():
            return
        self._start_screen()
        receiver = threading.Thread(target=self.process_received_data, daemon=True)
        receiver.start()
        try:
            self.handle_user_input()
        finally:
            if self.serial_conn and self.serial_conn.is_open:
                self.serial_conn.close()
            self._stop_screen()
            print("串口连接已关闭")
            print(f"最终统计:\n  接收字节数: {self.received_bytes}\n  发送字节数: {self.sent_bytes}\n程序结束")


def list_serial_ports() -> List[str]:
    ports: List[str] = []
    if sys.platform.startswith("win"):
        for number in range(1, 257):
            port = f"COM{number}"
            try:
                connection = serial.Serial(port)
                connection.close()
                ports.append(port)
            except (OSError, serial.SerialException):
                pass
    elif sys.platform.startswith("linux"):
        import glob
        ports = glob.glob("/dev/ttyUSB*") + glob.glob("/dev/ttyACM*") + glob.glob("/dev/ttyS*")
    elif sys.platform.startswith("darwin"):
        import glob
        ports = glob.glob("/dev/tty.usb*") + glob.glob("/dev/tty.*")
    return ports


def parse_log_path(value: str) -> Tuple[str, str]:
    if "=" not in value:
        raise argparse.ArgumentTypeError("日志路径格式应为 file=path")
    key, path = value.split("=", 1)
    if not key:
        raise argparse.ArgumentTypeError("file 不能为空")
    return key, path


def main() -> None:
    parser = argparse.ArgumentParser(description="增强版串口调试工具")
    parser.add_argument("port", nargs="?", help="串口号，如 COM6")
    parser.add_argument("-b", "--baudrate", type=int, default=9600)
    parser.add_argument("-e", "--encoding", default="utf-8")
    parser.add_argument("-l", "--list-ports", action="store_true")
    parser.add_argument("--log-path", action="append", type=parse_log_path,
                        metavar="FILE=PATH", help="配置日志文件字段到磁盘路径")
    parser.add_argument("--show-struct", action="store_true",
                        help="在控制台显示已接收的 Struct 原始字节")
    args = parser.parse_args()
    if args.list_ports:
        print("可用串口:", ", ".join(list_serial_ports()) or "无")
        return
    if not args.port:
        parser.error("请指定串口号，或使用 --list-ports")
    EnhancedSerialTool(args.port, args.baudrate, args.encoding,
                       dict(args.log_path or []), args.show_struct).run()


if __name__ == "__main__":
    main()
