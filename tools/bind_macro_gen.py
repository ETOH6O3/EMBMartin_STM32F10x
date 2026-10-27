

# with open("out.txt", "w") as f:
#     for i in range(0, 256):
#         print(f"#define GENERATE_BINDINGS_{i} ", file=f, end="")
#         for j in range(0, i):
#             print(f"PLACEHOLDER({j})", file=f, end="")
#             if j != (i - 1):
#                 print(",", file=f, end="")

#         print(file=f, end="\n")

with open("out.txt", "w") as f:
    for i in range(1, 257):
        pack_args: str = ",".join([f"_{i-1}" for i in range(1, i+1)])
        pack_args_move: str = ",".join([f"std::move(_{i-1})" for i in range(1, i+1)])
        print(
f"""
else if constexpr(tuple_size_v<TUPLE> == {i} )
{{
	const auto&[{pack_args}] = _tuple_obj;
	return std::make_tuple({pack_args_move});
}}
""", file=f, end="")
        
