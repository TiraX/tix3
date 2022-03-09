/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	namespace EShaderPrecisionModifier
	{
		enum Type
		{
			Float,
			Half,
			Fixed
		};
	};

	// Macros for declaring uniform buffer structures.

/** Declares a member of a uniform buffer struct. */
#define DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EXPLICIT(MemberType,MemberName,ArrayDecl,Precision) \
    typedef MemberType zzA##MemberName ArrayDecl; \
    zzA##MemberName MemberName;

#define DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(MemberType,MemberName) DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EXPLICIT(MemberType,MemberName,,EShaderPrecisionModifier::Float)
#define DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EX(MemberType,MemberName,Precision) DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EXPLICIT(MemberType,MemberName,,Precision)
#define DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_ARRAY(MemberType,MemberName,ArrayDecl) DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EXPLICIT(MemberType,MemberName,ArrayDecl,EShaderPrecisionModifier::Float)
#define DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_ARRAY_EX(MemberType,MemberName,ArrayDecl,Precision) DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EXPLICIT(MemberType,MemberName,ArrayDecl,Precision)

	/** Begins a uniform buffer struct declaration. */
#define BEGIN_UNIFORM_BUFFER_STRUCT_FIX_SIZE(StructTypeName,ArrayElements) \
	class StructTypeName : public IReferenceCounted \
	{ \
	public: \
		static const uint32 Elements = ArrayElements; \
		StructTypeName () { Data.resize(StructTypeName::Elements); }\
		struct FUniformBufferStruct \
		{ \

#define BEGIN_UNIFORM_BUFFER_STRUCT_DYNAMIC_SIZE(StructTypeName) \
	class StructTypeName : public IReferenceCounted \
	{ \
	public: \
		StructTypeName (uint32 ElementSize) { Data.resize(ElementSize); }\
		struct FUniformBufferStruct \
		{ \

#define END_UNIFORM_BUFFER_STRUCT(StructTypeName) \
		}; \
		uint32 GetElementsCount() const \
		{ \
			return (uint32)Data.size(); \
		} \
		uint32 GetStructureStrideInBytes() const \
		{ \
			return (uint32)sizeof(StructTypeName::FUniformBufferStruct); \
		} \
		TVector<FUniformBufferStruct> Data; \
		void InitToZero() \
		{ \
			memset(Data.data(), 0, GetStructureStrideInBytes() * GetElementsCount() ); \
		} \
		FUniformBufferPtr UniformBuffer; \
		FUniformBufferPtr InitUniformBuffer(FRHICmdList* CmdList, uint32 UBFlag = 0) \
		{ \
			TI_ASSERT(CmdList->WorkingThread == TThread::GetThreadId()); \
			FRHI * RHI = FRHI::Get(); \
			UniformBuffer = ti_new FUniformBuffer((uint32)sizeof(StructTypeName::FUniformBufferStruct), GetElementsCount(), UBFlag); \
			UniformBuffer->SetResourceName(#StructTypeName); \
			TStreamPtr UploadData = ti_new TStream(Data.data(), UniformBuffer->GetTotalBufferSize()); \
			UniformBuffer->CreateGPUBuffer(CmdList, UploadData); \
			return UniformBuffer; \
		} \
	}; \
	typedef TI_INTRUSIVE_PTR(StructTypeName) StructTypeName##Ptr;

#define BEGIN_UNIFORM_BUFFER_STRUCT(StructTypeName) BEGIN_UNIFORM_BUFFER_STRUCT_FIX_SIZE(StructTypeName,1)
#define BEGIN_UNIFORM_BUFFER_STRUCT_ARRAY(StructTypeName, ArrayElements) BEGIN_UNIFORM_BUFFER_STRUCT_FIX_SIZE(StructTypeName, ArrayElements)
#define BEGIN_UNIFORM_BUFFER_STRUCT_ARRAY_DYNAMIC(StructTypeName) BEGIN_UNIFORM_BUFFER_STRUCT_DYNAMIC_SIZE(StructTypeName)
	
	class TI_API FUniformBuffer : public FRenderResource
	{
	public:
		FUniformBuffer(uint32 InStructureSizeInBytes, uint32 InElements, uint32 InUBFlag = 0);
		virtual ~FUniformBuffer();

		virtual void CreateGPUBuffer(FRHICmdList* CmdList, TStreamPtr Data) override;
		virtual FGPUResourcePtr GetGPUResource() override
		{
			return Buffer;
		}
		virtual void PrepareDataForCPU(FRHICmdList* CmdList) {};
		virtual TStreamPtr ReadBufferData() { return nullptr; }

		FGPUBufferPtr GetGPUBuffer()
		{
			return Buffer;
		}
		uint32 GetTotalBufferSize() const
		{
			return StructureSizeInBytes * Elements;
		}
		uint32 GetStructureSizeInBytes() const
		{
			return StructureSizeInBytes;
		}
		uint32 GetElements() const
		{
			return Elements;
		}
		uint32 GetFlag() const
		{
			return UBFlag;
		}
		virtual uint32 GetCounterOffset() const
		{
			RuntimeFail();
			return 0;
		}
	protected:

	protected:
		uint32 StructureSizeInBytes;
		uint32 Elements;
		uint32 UBFlag;

		FGPUBufferPtr Buffer;
	};

	/////////////////////////////////////////////////////////////
	class TI_API FUniformBufferReadable : public FUniformBuffer
	{
	public:
		FUniformBufferReadable(uint32 InStructureSizeInBytes, uint32 Elements, uint32 InFlag);
		virtual ~FUniformBufferReadable();

		virtual void CreateGPUBuffer(FRHICmdList* CmdList, TStreamPtr Data) override;

		virtual void PrepareDataForCPU(FRHICmdList* CmdList) override;
		virtual TStreamPtr ReadBufferData() override;
	protected:
		FGPUBufferPtr ReadbackBuffer;
	};
}
