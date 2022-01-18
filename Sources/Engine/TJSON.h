/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

#include "document.h"
using namespace rapidjson;

namespace tix
{
	class TJSONNodeIterator
	{
	public:
		TJSONNodeIterator(Value::MemberIterator InIter)
			: Iter(InIter)
		{}

		TJSONNodeIterator& operator ++ ()
		{
			++Iter;
			return *this;
		}

		bool operator != (const TJSONNodeIterator& Other)
		{
			return Iter != Other.Iter;
		}

		const int8* Name()
		{
			return Iter->name.GetString();
		}

	private:
		Value::MemberIterator Iter;
	};

	class TJSONNode
	{
	public:
		TJSONNode()
			: JsonValue(nullptr)
		{}

		TJSONNode(Value* InValue)
			: JsonValue(InValue)
		{}

		TJSONNode operator[] (const int8* Name) const
		{
			if (IsNull())
				return TJSONNode();

			Value::MemberIterator Member = JsonValue->FindMember(Name);
			if (Member != JsonValue->MemberEnd())
			{
				TJSONNode Node(&(*JsonValue)[Name]);
				return Node;
			}

			return TJSONNode();
		}

		TJSONNode operator[] (int32 Index) const
		{
			if (IsNull())
				return TJSONNode();

			TJSONNode Node(&(*JsonValue)[Index]);
			return Node;
		}

		TJSONNodeIterator MemberBegin()
		{
			TI_ASSERT(!IsNull());
			TJSONNodeIterator It(JsonValue->MemberBegin());
			return It;
		}

		TJSONNodeIterator MemberEnd()
		{
			TI_ASSERT(!IsNull());
			TJSONNodeIterator It(JsonValue->MemberEnd());
			return It;
		}

		bool IsNull() const
		{
			return JsonValue == nullptr;
		}

		bool IsArray() const
		{
			if (IsNull())
				return false;

			return JsonValue->IsArray();
		}

		bool IsObject() const
		{
			if (IsNull())
				return false;

			return JsonValue->IsObject();
		}

		bool IsString() const
		{
			if (IsNull())
				return false;

			return JsonValue->IsString();
		}

		int32 Size() const
		{
			if (IsNull())
				return 0;

			return (int32)JsonValue->Size();
		}

		void operator << (bool& OutBool) const
		{
			if (!IsNull())
				OutBool = JsonValue->GetBool();
		}

		void operator << (TString& OutString) const
		{
			if (!IsNull())
				OutString = JsonValue->GetString();
		}

		void operator << (int32& OutInt) const
		{
			if (!IsNull())
				OutInt = JsonValue->GetInt();
		}

		void operator << (float& OutFloat) const
		{
			if (!IsNull())
				OutFloat = JsonValue->GetFloat();
		}

		void operator << (FInt2& OutVec2) const
		{
			TI_ASSERT(IsNull() || (IsArray() && Size() == 2));
			if (!IsNull())
			{
				(*this)[0] << OutVec2.X;
				(*this)[1] << OutVec2.Y;
			}
		}

		void operator << (FFloat2& OutVec2) const
		{
			TI_ASSERT(IsNull() || (IsArray() && Size() == 2));
			if (!IsNull())
			{
				(*this)[0] << OutVec2.X;
				(*this)[1] << OutVec2.Y;
			}
		}

		void operator << (FFloat3& OutVec3) const
		{
			TI_ASSERT(IsNull() || (IsArray() && Size() == 3));
			if (!IsNull())
			{
				(*this)[0] << OutVec3.X;
				(*this)[1] << OutVec3.Y;
				(*this)[2] << OutVec3.Z;
			}
		}

		void operator << (FQuat& OutQuat) const
		{
			TI_ASSERT(IsNull() || (IsArray() && Size() == 4));
			if (!IsNull())
			{
				(*this)[0] << OutQuat.X;
				(*this)[1] << OutQuat.Y;
				(*this)[2] << OutQuat.Z;
				(*this)[3] << OutQuat.W;
			}
		}

		void operator << (FBox& OutBBox) const
		{
			TI_ASSERT(IsNull() || (IsArray() && Size() == 6));
			if (!IsNull())
			{
				(*this)[0] << OutBBox.Min.X;
				(*this)[1] << OutBBox.Min.Y;
				(*this)[2] << OutBBox.Min.Z;
				(*this)[3] << OutBBox.Max.X;
				(*this)[4] << OutBBox.Max.Y;
				(*this)[5] << OutBBox.Max.Z;
			}
		}

		void operator << (SColorf& OutColorf) const
		{
			TI_ASSERT(IsNull() || (IsArray() && Size() == 4));
			if (!IsNull())
			{
				(*this)[0] << OutColorf.R;
				(*this)[1] << OutColorf.G;
				(*this)[2] << OutColorf.B;
				(*this)[3] << OutColorf.A;
			}
		}

		void operator << (TVector<TString>& OutArray) const
		{
			TI_ASSERT(IsNull() || IsArray());
			if (!IsNull())
			{
				OutArray.reserve(OutArray.size() + Size());
				for (int32 i = 0; i < Size(); i++)
				{
					TString S;
					(*this)[i] << S;
					OutArray.push_back(S);
				}
			}
		}

		void operator << (TVector<int32>& OutArray) const
		{
			TI_ASSERT(IsNull() || IsArray());
			if (!IsNull())
			{
				OutArray.reserve(OutArray.size() + Size());
				for (int32 i = 0; i < Size(); i++)
				{
					int32 N;
					(*this)[i] << N;
					OutArray.push_back(N);
				}
			}
		}

		void operator << (TVector<float>& OutArray) const
		{
			TI_ASSERT(IsNull() || IsArray());
			if (!IsNull())
			{
				OutArray.reserve(OutArray.size() + Size());
				for (int32 i = 0; i < Size(); i++)
				{
					float N;
					(*this)[i] << N;
					OutArray.push_back(N);
				}
			}
		}

	protected:
		Value* JsonValue;
	};

	class TJSON : public TJSONNode
	{
	public:
		TJSON()
		{}

		~TJSON()
		{}

		void Parse(const int8* JsonText)
		{
			JsonDoc.Parse(JsonText);
			JsonValue = &JsonDoc;
		}

	protected:
		Document JsonDoc;
	};
}