/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

class FScatterUploadCS : public FComputeTask
{
public:
	FScatterUploadCS();
	virtual ~FScatterUploadCS();

	void Reset(uint32 Count, FUniformBufferPtr InDstBuffer);
	void Add(uint32 Address, uint32 Value);
	virtual void Run(FRHICmdList* RHICmdList) override;

	enum
	{
		RC_ScatterInfo,
		SRV_ScatterAddresses,
		SRV_ScatterValues,

		UAV_DstPageBuffer,

		PARAM_NUM,
	};

	struct FScatterInfo
	{
		uint32 Count;
		FScatterInfo()
			: Count(0)
		{}
	};


private:
	FScatterInfo ScatterInfo;

private:
	TVector<uint32> Addresses;
	TVector<uint32> Values;

	FUniformBufferPtr UB_Addr;
	FUniformBufferPtr UB_Value;
	FUniformBufferPtr DstBuffer;

};
typedef TI_INTRUSIVE_PTR(FScatterUploadCS) FScatterUploadCSPtr;
