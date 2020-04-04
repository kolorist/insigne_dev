namespace mat_parser
{
// ----------------------------------------------------------------------------

template <class TMemoryArena>
const MaterialDescription ParseMaterial(const_cstr i_descBuffer, TMemoryArena* i_memoryArena)
{
	MaterialDescription material;

	internal::TokenArray<TMemoryArena> tokenArray(i_memoryArena);

	const_cstr lexResult = MaterialLexer(i_descBuffer, &tokenArray, i_memoryArena);
	FLORAL_ASSERT(lexResult == nullptr);

	return material;
}

namespace internal
{
// ----------------------------------------------------------------------------

template <class TMemoryArena>
struct StringAllocatorRegistry
{
	static TMemoryArena* allocator;
	static voidptr allocate(const size i_size)
	{
		return allocator->allocate(i_size);
	}
};

template <class TMemoryArena>
TMemoryArena* StringAllocatorRegistry<TMemoryArena>::allocator = nullptr;

template <class TMemoryArena>
const_cstr MaterialLexer(const_cstr i_descBuffer, TokenArray<TMemoryArena>* o_tokenArray, TMemoryArena* i_memoryArena)
{
	StringAllocatorRegistry<TMemoryArena>::allocator = i_memoryArena;
	Tokenizer tokenizer(i_descBuffer, &StringAllocatorRegistry<TMemoryArena>::allocate);
	Token token;

	do
	{
		token = tokenizer.GetNextToken();
		if (token.type != TokenType::Invalid)
		{
			o_tokenArray->push_back(token);
		}
	} while (token.type != TokenType::Invalid);

	return nullptr;
}

// ----------------------------------------------------------------------------
}

// ----------------------------------------------------------------------------
}
