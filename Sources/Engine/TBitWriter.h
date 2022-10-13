/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	// Naive bit writer for cooking purposes
	class TBitWriter
	{
	public:
		TBitWriter(TVector<uint8>& InBuffer) :
			Buffer(InBuffer),
			PendingBits(0ull),
			NumPendingBits(0)
		{
		}

		void PutBits(uint32 Bits, uint32 NumBits)
		{
			TI_ASSERT((uint64)Bits < (1ull << NumBits));
			PendingBits |= (uint64)Bits << NumPendingBits;
			NumPendingBits += NumBits;

			while (NumPendingBits >= 8)
			{
				Buffer.push_back((uint8)PendingBits);
				PendingBits >>= 8;
				NumPendingBits -= 8;
			}
		}

		void Flush(uint32 Alignment = 1)
		{
			if (NumPendingBits > 0)
				Buffer.push_back((uint8)PendingBits);
			while (Buffer.size() % Alignment != 0)
				Buffer.push_back(0);
			PendingBits = 0;
			NumPendingBits = 0;
		}

	private:
		TVector<uint8>& Buffer;
		uint64 PendingBits;
		int32 NumPendingBits;
	};
}