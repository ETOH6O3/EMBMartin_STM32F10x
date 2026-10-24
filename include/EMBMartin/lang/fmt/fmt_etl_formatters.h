#pragma once

// fmt 组件对 ETL 扩展点的接入（自 fmt.h 拆出）：
//   - etl::formatter<T> 的广义 pair / fixed<N> 特化
//   - private_format::arg_type_mask 的收窄特化

#include "EMBMartin/lang/fmt/fmt_config.h"
#include "EMBMartin/lang/fmt/fmt_helpers.h"


namespace etl
{
	/**
	 * @brief 广义 pair 的 ETL 自定义格式化器
	 *
	 * @details 当 @c _Pair 是可解包的广义 pair 时，本特化选中并把两个元素按各自的子说明符
	 * 分别格式化，拼成 @c "[x,@c y]" 形式（括号与分隔符由说明符决定）。
	 *
	 * @tparam _Pair 广义 pair 类型
	 *
	 * @note 关于特化的写法：@c etl::formatter 的第二个模板参数是**字符类型**（默认 @c char ），
	 *       不是 @c std::enable_if 的落点。实测（armclang 6.24）：
	 *       - @c "formatter<T,@c char>"             → 能匹配，但无法约束；
	 *       - @c "formatter<T,@c enable_if_t<...>>" → 落点是 @c void ，**永远匹配不上**（ETL 只用 @c char ）；
	 *       - @c "formatter<T,@c conditional_t<cond,@c char,@c 不完整类型>>" → 既能匹配又能约束。
	 *       故这里用第三种：条件成立时槽位是 @c char ，否则替换失败（SFINAE）。
	 *
	 * @see EMBMartin::pair_spec
	 */
	template <typename _Pair>
	struct formatter<_Pair,
					 etl::conditional_t<::EMBMartin::pair_spec::is_decomposable_pair<_Pair>::value,
										char,
										::EMBMartin::pair_spec::not_a_pair>>
	{
		using _Tx = ::EMBMartin::generalized_pair_element_t<0, _Pair>; ///< 左元素类型
		using _Ty = ::EMBMartin::generalized_pair_element_t<1, _Pair>; ///< 右元素类型

		using _Spec = etl::private_format::format_spec_t; ///< ETL 的说明符类型

		::EMBMartin::pair_spec::parts _parts{}; ///< 切分后的 pair 说明符
		_Spec _spec_x{};						///< 左元素的说明符（仅在 @c _has_element_spec 为真时有效）
		_Spec _spec_y{};						///< 右元素的说明符（仅在 @c _has_element_spec 为真时有效）
		bool _has_element_spec = false;			///< 是否显式给出了元素级子说明符

		/**
		 * @brief 解析 pair 的说明符
		 *
		 * @param parse_ctx ETL 的解析上下文，指向 @c ':' 之后、@c '}' 之前
		 * @return 指向收尾 @c '}' 的迭代器
		 */
		etl::format_parse_context::iterator parse(etl::format_parse_context &parse_ctx)
		{
			// 本替换字段的收尾 '}'（广义 pair 说明符里不会出现嵌套的 {} ）
			auto begin = parse_ctx.begin();
			auto end = parse_ctx.end();
			auto close = begin;
			while (close != end && *close != '}')
			{
				++close;
			}

			// 切分广义 pair 说明符
			::EMBMartin::pair_spec::parts parts;
			::EMBMartin::pair_spec::split(etl::string_view(begin, static_cast<size_t>(close - begin)), parts);
			_parts = parts;
			_has_element_spec = ::EMBMartin::pair_spec::has_element_spec(parts);

			// 元素子说明符：复用 ETL 的说明符解析器 + 元素自己的 formatter
			if (_has_element_spec)
			{
				_parse_element(parts.x, _spec_x, _formatter_x);
				_parse_element(parts.y, _spec_y, _formatter_y);
			}
			// 没有元素级说明符时不需要 _spec_x/_spec_y：format 里直接用外层说明符

			return close;
		}

		/**
		 * @brief 格式化一对值
		 *
		 * @tparam OutputIt 输出迭代器类型
		 * @param value   被格式化的 pair
		 * @param fmt_ctx ETL 的格式上下文
		 * @return 已推进的输出迭代器
		 *
		 * @note 处理完两个元素后会恢复外层说明符（@c fmt_ctx.format_spec = outer ），
		 *       以免影响后续替换字段。
		 */
		template <typename OutputIt>
		typename etl::format_context<OutputIt>::iterator
		format(const _Pair &value, etl::format_context<OutputIt> &fmt_ctx)
		{
			const auto &[x, y] = value;
			const _Spec outer = fmt_ctx.format_spec;
			OutputIt out = fmt_ctx.out();

			if (_parts.prefix != '\0')
			{
				*out = _parts.prefix;
				++out;
				fmt_ctx.advance_to(out);
			}

			fmt_ctx.format_spec = _has_element_spec ? _spec_x : outer;
			out = _formatter_x.format(x, fmt_ctx);
			fmt_ctx.advance_to(out);

			*out = _parts.sep;
			++out;
			*out = ' ';
			++out;
			fmt_ctx.advance_to(out);

			fmt_ctx.format_spec = _has_element_spec ? _spec_y : outer;
			out = _formatter_y.format(y, fmt_ctx);
			fmt_ctx.advance_to(out);

			if (_parts.suffix != '\0')
			{
				*out = _parts.suffix;
				++out;
				fmt_ctx.advance_to(out);
			}

			fmt_ctx.format_spec = outer;
			return out;
		}

	private:
		etl::formatter<_Tx> _formatter_x{}; ///< 左元素的格式化器
		etl::formatter<_Ty> _formatter_y{}; ///< 右元素的格式化器

		/**
		 * @brief 解析一个元素子说明符
		 *
		 * @tparam Formatter 该元素的 formatter 类型
		 * @param spec              子说明符文本
		 * @param out               [out] 解析出的 ETL 说明符
		 * @param element_formatter 该元素的 formatter，解析后供 @c format 使用
		 *
		 * @note 完全照搬 ETL 的顺序：先用 ETL 自己的 @c parse_format_spec 吃掉标准片段，
		 *       再把剩余部分交给元素自己的 formatter（内置 formatter 的 @c parse 是空操作）。
		 */
		template <typename Formatter>
		static void _parse_element(etl::string_view spec, _Spec &out, Formatter &element_formatter)
		{
			etl::format_parse_context sub_ctx(spec, 1);
			etl::private_format::parse_format_spec(sub_ctx, out);
			element_formatter.parse(sub_ctx);
		}
	};

	/**
	 * @brief  @ref EMBMartin::fixed "fixed<N>" 在 ETL 后端下的自定义格式化器
	 *
	 * @details
	 * ETL 已把它认识的说明符片段解析进 `fmt_ctx.format_spec`（fill / align / sign /
	 * `0` / width / precision / type），本特化只把这些字段翻译成
	 * 渲染交给与旧后端共用的整数运算实现 @c EMBMartin::fixed_detail::spec 。
	 * 因此 ETL 那个「浮点 precision 被忽略」的缺陷不会影响 `fixed<N>`。
	 *
	 * 支持的表示类型只有 `f` 与 `F`；给出 `e` / `g` / `a` 等其它表示类型会触发
	 * @c EMBMARTIN_FMT_ASSERT。
	 *
	 * @tparam N 默认小数位数，见 @ref EMBMartin::fixed
	 */
	template <size_t N>
	struct formatter<::EMBMartin::fixed<N>, char>
	{
		/**
		 * @brief 消费本替换字段的说明符
		 *
		 * @param parse_ctx 解析上下文，指向 `:` 之后、`}` 之前
		 * @return 指向收尾 `}` 的迭代器
		 *
		 * @note `fixed<N>` 没有自己的额外说明符语法，故把 ETL 吃剩的部分整体消费到 `}`
		 *       即可；若用户给出定点以外的表示类型，统一在 @ref format 中断言拒绝。
		 */
		etl::format_parse_context::iterator parse(etl::format_parse_context &parse_ctx)
		{
			auto it = parse_ctx.begin();
			while (it != parse_ctx.end() && *it != '}')
			{
				++it;
			}
			return it;
		}

		/**
		 * @brief 渲染 `fixed<N>` 的值
		 *
		 * @tparam OutputIt 输出迭代器类型
		 * @param value   被格式化的值
		 * @param fmt_ctx 格式上下文，其中 `format_spec` 已由 ETL 解析好
		 * @return 已推进的输出迭代器
		 */
		template <typename OutputIt>
		typename format_context<OutputIt>::iterator format(const ::EMBMartin::fixed<N> &value,
														   format_context<OutputIt> &fmt_ctx)
		{
			using spec_t = etl::private_format::format_spec_t;

			const spec_t &fs = fmt_ctx.format_spec;

			if (fs.type.has_value() && fs.type.value() != 'f' && fs.type.value() != 'F')
			{
				// 只支持 f / F：e / g / a 等定点以外的表示类型没有意义
				EMBMARTIN_FMT_ASSERT(fs.type.value() == 'f' || fs.type.value() == 'F');
			}

			::EMBMartin::fixed_detail::spec s;
			s.fill = fs.fill;
			s.zero_fill = fs.zero;
			s.upper = fs.type.has_value() && fs.type.value() == 'F';
			s.precision = fs.precision.has_value() ? static_cast<int>(fs.precision.value()) : -1;
			s.width = fs.width.has_value() ? static_cast<int>(fs.width.value()) : 0;

			switch (fs.align)
			{
			case etl::private_format::spec_align_t::START:
				s.align = '<';
				break;
			case etl::private_format::spec_align_t::CENTER:
				s.align = '^';
				break;
			default: // END / NONE：数字默认右对齐
				s.align = '>';
				break;
			}
			switch (fs.sign)
			{
			case etl::private_format::spec_sign_t::PLUS:
				s.sign = '+';
				break;
			case etl::private_format::spec_sign_t::SPACE:
				s.sign = ' ';
				break;
			default:
				s.sign = '-';
				break;
			}

			char buf[::EMBMartin::fixed_detail::buffer_size];
			const size_t written = ::EMBMartin::fixed_detail::render(buf, value.value, static_cast<unsigned>(N), s);

			OutputIt out = fmt_ctx.out();
			for (size_t i = 0; i < written; ++i)
			{
				*out = buf[i];
				++out;
			}
			fmt_ctx.advance_to(out);
			return out;
		}
	};

	namespace private_format
	{
		/**
		 * @brief  为「自定义可格式化类型」收窄 ETL 的编译期参数掩码
		 *
		 * @details
		 * **问题**：ETL 的 `basic_format_arg` 用一个含 21 个候选的 variant 存放实参，
		 * `etl::visit` 会遍历其中每一个候选。为免实例化用不到的 formatter（尤其整条浮点
		 * 链），ETL 用 `arg_type_mask<T>` 在编译期算出「本次调用可能命中哪些候选」，再由
		 * `format_visitor<OutputIt, Mask>` 把不可达候选的 `dispatch` 编成空函数。
		 *
		 * 然而 `arg_type_mask` 只为 21 个内建类型及 `etl::ibasic_string` 的派生类提供了
		 * 特化；对**其它任何类型**，主模板保守地返回 `mask_all`：
		 * @code
		 * template <typename T, typename Enable = void>
		 * struct arg_type_mask { static constexpr arg_mask_t value = mask_all; };
		 * @endcode
		 * 于是只要一次格式化调用中出现自定义类型（本仓库的 `Coordinate`、`add_result`、
		 * 广义 pair 等），该次调用的 Mask 便退化为 `mask_all`，ETL 会把 **21 个内建分支的
		 * formatter 全部实例化**——包括 `format_floating_*` 整条链及其拖入的
		 * `pow` / `log` / `log10` 等数学库函数。这是本组件接入 ETL 后代码体积明显增大的
		 * 真正原因（并非两套后端被同时实例化）。
		 *
		 * **为何收窄是安全的**：通过 `etl::formatter<T>` 特化接入的自定义类型，在
		 * `basic_format_arg` 中存放于 **`handle` 槽**（见 ETL 的 converting constructor），
		 * 格式化时走 `format_visitor::operator()(handle)` 这一**非模板重载**，完全不经过
		 * 内建分支的 `dispatch`。即：对该类型而言 21 个内建分支一个都用不到，`mask_all`
		 * 中除 `mask_monostate`（掩码递归的下界、恒置位）之外全是浪费。
		 *
		 * 因此这里把「所有提供了 `etl::formatter<T>` 特化的类型」统一收窄到
		 * `mask_monostate`。它与 ETL 自带的 `ibasic_string` 特化互斥：后者只对
		 * `etl::ibasic_string<char>` 的派生类（`etl::string` / `etl::istring` 等）生效，
		 * 而那些类型在 `basic_format_arg` 中存放于 `string_view` 槽、走内建分支；本特化
		 * 只接管真正走 `handle` 槽的类型，二者不会同时对同一类型成立。
		 *
		 * @par 实测效果
		 * Keil armclang V6.24，本仓库 `martin_test/main.cpp` 当前版本：
		 * - 未收窄：`Code = 59396` B
		 * - 收窄后：`Code = 37252` B（节省 22144 B）
		 * - 对照：同一份代码的旧后端为 `Code = 42520` B
		 *
		 * @note 本特化只让编译器不生成用不到的代码，**不改变任何格式化结果**。
		 * @warning 这里特化的是 ETL 的内部扩展点。若将来 ETL 改为「自定义类型也走内建槽」，
		 *          本特化需要重新评估。
		 */
		template <typename T>
		struct arg_type_mask<T, etl::enable_if_t<is_formattable<T>::value>>
		{
			/// @brief 仅保留 `mask_monostate` 位，裁掉全部内建分支
			static constexpr arg_mask_t value = mask_monostate;
		};
	} // namespace private_format
} // namespace etl
