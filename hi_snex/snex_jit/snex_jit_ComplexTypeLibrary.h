/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#pragma once

namespace snex {
namespace jit {
using namespace juce;


struct WrapType : public ComplexType
{
	enum OpType
	{
		Inc,
		Dec,
		Set,
		numOpTypes
	};

	WrapType(int size_);

	size_t getRequiredByteSize() const override { return 4; }
	virtual size_t getRequiredAlignment() const override { return 0; }
	void dumpTable(juce::String& s, int& intentLevel, void* dataStart, void* complexTypeStartPointer) const override;
	FunctionClass* getFunctionClass() override;
	InitialiserList::Ptr makeDefaultInitialiserList() const override;
	Result initialise(InitData data) override;
	bool forEach(const TypeFunction&, Ptr, void*) override { return false; }
	juce::String toStringInternal() const override;


	bool isValidCastTarget(Types::ID nativeTargetType, ComplexType::Ptr complexTargetType) const override;

	const int size;
};

struct IndexBase : public ComplexType
{
	IndexBase(const TypeInfo& parentType);

	virtual Identifier getIndexName() const = 0;

	virtual Inliner::Func getAsmFunction(FunctionClass::SpecialSymbols s);

	virtual int getInitValue(int input) const { return input; }

	FunctionData* createOperator(FunctionClass* f, FunctionClass::SpecialSymbols s)
	{
		if (auto asmFunc = getAsmFunction(s))
		{
			auto op = new FunctionData();
			op->returnType = TypeInfo(this);
			op->id = f->getClassName().getChildId(f->getSpecialSymbol(s));
			op->inliner = new Inliner(op->id, asmFunc, {});
			f->addFunction(op);

			return op;
		}

		return nullptr;
	}

	size_t getRequiredByteSize() const override { return 4; }
	size_t getRequiredAlignment() const override { return 0; };

	bool forEach(const TypeFunction& t, ComplexType::Ptr typePtr, void* dataPointer) override;

	bool isValidCastSource(Types::ID nativeSourceType, ComplexType::Ptr complexSourceType) const override;
	bool isValidCastTarget(Types::ID nativeTargetType, ComplexType::Ptr complexTargetType) const override;

	Types::ID getRegisterType() const override { return Types::ID::Integer; }

	InitialiserList::Ptr makeDefaultInitialiserList() const override;

	FunctionClass* getFunctionClass() override;

	Result initialise(InitData data) override;

	void dumpTable(juce::String& s, int& intendLevel, void* dataStart, void* complexTypeStartPointer) const override;

	juce::String toStringInternal() const override;

	ComplexType::WeakPtr parentType;
};

struct ArrayTypeBase : public ComplexType
{
	virtual ~ArrayTypeBase() {}

	virtual TypeInfo getElementType() const = 0;
};

struct SpanType : public ArrayTypeBase
{
	struct Wrapped : public IndexBase
	{
		Wrapped(TypeInfo p) :
			IndexBase(p)
		{};

		Identifier getIndexName() const override { RETURN_STATIC_IDENTIFIER("wrapped"); }
		Inliner::Func getAsmFunction(FunctionClass::SpecialSymbols s) override;
		
		int getInitValue(int input) const { return input % getSpanSize(); }
		int getSpanSize() const { return dynamic_cast<SpanType*>(parentType.get())->getNumElements(); }
	};

	struct Unsafe : public IndexBase
	{
		Unsafe(TypeInfo p):
			IndexBase(p)
		{}

		Identifier getIndexName() const override { RETURN_STATIC_IDENTIFIER("unsafe"); };
	};

	/** Creates a simple one-dimensional span. */
	SpanType(const TypeInfo& dataType, int size_);

	void finaliseAlignment() override;
	size_t getRequiredByteSize() const override;
	size_t getRequiredAlignment() const override;
	InitialiserList::Ptr makeDefaultInitialiserList() const override;
	void dumpTable(juce::String& s, int& intendLevel, void* dataStart, void* complexTypeStartPointer) const override;
	juce::String toStringInternal() const override;
	Result initialise(InitData data) override;
	bool forEach(const TypeFunction& t, ComplexType::Ptr typePtr, void* dataPointer) override;

	FunctionClass* getFunctionClass() override;
	TypeInfo getElementType() const override;
	int getNumElements() const;
	static bool isSimdType(const TypeInfo& t);
	size_t getElementSize() const;

	ComplexType::Ptr createSubType(const NamespacedIdentifier& id) override;

private:

	TypeInfo elementType;
	juce::String typeName;
	int size;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpanType);
};

struct DynType : public ArrayTypeBase
{
	enum class IndexType
	{
		W,
		C,
		Z,
		U,
		numIndexTypes
	};

	struct Wrapped : public IndexBase
	{
		Wrapped(TypeInfo p) :
			IndexBase(p)
		{};

		Identifier getIndexName() const override { RETURN_STATIC_IDENTIFIER("wrapped"); }
	};

	struct Unsafe : public IndexBase
	{
		Unsafe(TypeInfo p) :
			IndexBase(p)
		{}

		Identifier getIndexName() const override { RETURN_STATIC_IDENTIFIER("unsafe"); };
	};

	DynType(const TypeInfo& elementType_);

	static IndexType getIndexType(const TypeInfo& t);

	TypeInfo getElementType() const override { return elementType; }

	size_t getRequiredByteSize() const override;
	virtual size_t getRequiredAlignment() const override;
	void dumpTable(juce::String& s, int& intentLevel, void* dataStart, void* complexTypeStartPointer) const;
	FunctionClass* getFunctionClass() override;
	InitialiserList::Ptr makeDefaultInitialiserList() const override;
	Result initialise(InitData data) override;
	bool forEach(const TypeFunction&, Ptr, void*) override { return false; }
	juce::String toStringInternal() const override;

	ComplexType::Ptr createSubType(const NamespacedIdentifier& id) override;

	TypeInfo elementType;

private:

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DynType);
};

struct StructType : public ComplexType
{
	StructType(const NamespacedIdentifier& s);;

	size_t getRequiredByteSize() const override;
	size_t getRequiredAlignment() const override;
	void finaliseAlignment() override;
	juce::String toStringInternal() const override;
	FunctionClass* getFunctionClass() override;
	Result initialise(InitData data) override;
	bool forEach(const TypeFunction& t, ComplexType::Ptr typePtr, void* dataPointer) override;
	void dumpTable(juce::String& s, int& intendLevel, void* dataStart, void* complexTypeStartPointer) const override;
	InitialiserList::Ptr makeDefaultInitialiserList() const override;
	
	void registerExternalAtNamespaceHandler(NamespaceHandler* handler);

	bool setDefaultValue(const Identifier& id, InitialiserList::Ptr defaultList);

	bool hasMember(const Identifier& id) const;
	TypeInfo getMemberTypeInfo(const Identifier& id) const;
	Types::ID getMemberDataType(const Identifier& id) const;
	bool isNativeMember(const Identifier& id) const;
	ComplexType::Ptr getMemberComplexType(const Identifier& id) const;
	size_t getMemberOffset(const Identifier& id) const;
	void addJitCompiledMemberFunction(const FunctionData& f);

	bool injectMemberFunctionPointer(const FunctionData& f, void* fPointer);

	template <class ObjectType, typename ArgumentType> void addExternalComplexMember(const Identifier& id, ComplexType::Ptr p, ObjectType& obj, ArgumentType& defaultValue)
	{
		auto nm = new Member();
		nm->id = id;
		nm->typeInfo = TypeInfo(p);
		nm->offset = reinterpret_cast<uint64>(&defaultValue) - reinterpret_cast<uint64>(&obj);
		nm->defaultList = p->makeDefaultInitialiserList();

		memberData.add(nm);
		isExternalDefinition = true;
	}

	template <class ObjectType, typename ArgumentType> void addExternalMember(const Identifier& id, ObjectType& obj, ArgumentType& defaultValue)
	{
		auto type = Types::Helpers::getTypeFromTypeId<ArgumentType>();
		auto nm = new Member();
		nm->id = id;
		nm->typeInfo = TypeInfo(type);
		nm->offset = reinterpret_cast<uint64>(&defaultValue) - reinterpret_cast<uint64>(&obj);
		nm->defaultList = InitialiserList::makeSingleList(VariableStorage(type, var(defaultValue)));

		memberData.add(nm);
		isExternalDefinition = true;
	}

	void addMember(const Identifier& id, const TypeInfo& typeInfo, size_t offset = 0)
	{
		jassert(!isFinalised());
		auto nm = new Member();
		nm->id = id;
		nm->typeInfo = typeInfo;
		nm->offset = 0;
		memberData.add(nm);
	}

	template <typename ReturnType, typename... Parameters>void addExternalMemberFunction(const Identifier& id, ReturnType(*ptr)(Parameters...))
	{
		FunctionData f = FunctionData::create(id, ptr, true);
		f.function = ptr;

		memberFunctions.add(f);
	}

	NamespacedIdentifier id;

private:

	Array<FunctionData> memberFunctions;

	struct Member
	{
		size_t offset = 0;
		size_t padding = 0;
		Identifier id;
		TypeInfo typeInfo;
		InitialiserList::Ptr defaultList;
	};

	static void* getMemberPointer(Member* m, void* dataPointer);

	static size_t getRequiredAlignment(Member* m);

	OwnedArray<Member> memberData;
	bool isExternalDefinition = false;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StructType);
};


struct VariadicTypeBase : public ComplexType
{
	VariadicTypeBase(VariadicSubType::Ptr subType);;

	virtual ~VariadicTypeBase() {};

	static VariadicTypeBase* getVariadicObjectFromInlineData(InlineData* d);

	int getNumSubTypes() const;
	void addType(ComplexType::Ptr newType);
	size_t getOffsetForSubType(int index) const;
	ComplexType::Ptr getSubType(int index) const;

	size_t getRequiredByteSize() const override;
	size_t getRequiredAlignment() const override;
	void finaliseAlignment() override;;
	void dumpTable(juce::String& s, int& intentLevel, void* dataStart, void* complexTypeStartPointer) const override;
	Result initialise(InitData data) override;
	InitialiserList::Ptr makeDefaultInitialiserList() const override;
	bool forEach(const TypeFunction& t, Ptr typePtr, void* dataPointer) override;
	FunctionClass* getFunctionClass() override;
	juce::String toStringInternal() const override;;

	NamespacedIdentifier getVariadicId() const { return type->variadicId; }

private:

	VariadicSubType::Ptr type;
	ReferenceCountedArray<ComplexType> types;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VariadicTypeBase);
};





#define CREATE_SNEX_STRUCT(x) new StructType(NamespacedIdentifier(#x));
#define ADD_SNEX_STRUCT_MEMBER(structType, object, member) structType->addExternalMember(#member, object, object.member);
#define ADD_SNEX_STRUCT_COMPLEX(structType, typePtr, object, member) structType->addExternalComplexMember(#member, typePtr, object, object.member);

#define ADD_SNEX_STRUCT_METHOD(structType, obj, name) structType->addExternalMemberFunction(#name, obj::Wrapper::name);

#define ADD_INLINER(x, f) fc->addInliner(#x, [obj](InlineData* d_)f);

#define SETUP_INLINER(X) auto d = d_->toAsmInlineData(); auto& cc = d->gen.cc; auto base = x86::ptr(PTR_REG_R(d->object)); auto type = Types::Helpers::getTypeFromTypeId<X>();




}
}