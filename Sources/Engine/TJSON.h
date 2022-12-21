/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

#include "document.h"
#include "prettywriter.h"
#include "stringbuffer.h"

using namespace rapidjson;

namespace tix
{
	typedef Document::AllocatorType TJSONAllocator;

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

		int32 IntValue()
		{
			return Iter->value.GetInt();
		}

	private:
		Value::MemberIterator Iter;
	};
	/////////////////////////////////////////////////////////////

	class TJSONNode
	{
	public:
		TJSONNode()
			: JsonValue(nullptr)
			, Allocator(nullptr)
		{}

		TJSONNode(Value* InValue, TJSONAllocator* InAllocator)
			: JsonValue(InValue)
			, Allocator(InAllocator)
		{}

		virtual ~TJSONNode()
		{}

		TJSONNode operator[] (const int8* Name) const
		{
			if (IsNull())
				return TJSONNode();

			Value::MemberIterator Member = JsonValue->FindMember(Name);
			if (Member != JsonValue->MemberEnd())
			{
				TJSONNode Node(&(*JsonValue)[Name], Allocator);
				return Node;
			}

			return TJSONNode();
		}

		TJSONNode operator[] (int32 Index) const
		{
			if (IsNull())
				return TJSONNode();

			TJSONNode Node(&(*JsonValue)[Index], Allocator);
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

		// Read interfaces
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

		void operator << (FInt3& OutVec3) const
		{
			TI_ASSERT(IsNull() || (IsArray() && Size() == 3));
			if (!IsNull())
			{
				(*this)[0] << OutVec3.X;
				(*this)[1] << OutVec3.Y;
				(*this)[2] << OutVec3.Z;
			}
		}

		void operator << (FInt4& OutVec4) const
		{
			TI_ASSERT(IsNull() || (IsArray() && Size() == 4));
			if (!IsNull())
			{
				(*this)[0] << OutVec4.X;
				(*this)[1] << OutVec4.Y;
				(*this)[2] << OutVec4.Z;
				(*this)[3] << OutVec4.W;
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

		void operator << (FFloat4& OutVec4) const
		{
			TI_ASSERT(IsNull() || (IsArray() && Size() == 4));
			if (!IsNull())
			{
				(*this)[0] << OutVec4.X;
				(*this)[1] << OutVec4.Y;
				(*this)[2] << OutVec4.Z;
				(*this)[3] << OutVec4.W;
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
			if (!IsNull())
			{
				if (IsArray())
				{
					OutArray.reserve(OutArray.size() + Size());
					for (int32 i = 0; i < Size(); i++)
					{
						TString S;
						(*this)[i] << S;
						OutArray.push_back(S);
					}
				}
				else
				{
					OutArray.reserve(OutArray.size() + 1);
					TString S;
					(*this) << S;
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

		// Construct interfaces
		void AddMember(const char* Name, int32 V)
		{
			JsonValue->AddMember(StringRef(Name), V, *Allocator);
		}
		void AddMember(const char* Name, uint32 V)
		{
			JsonValue->AddMember(StringRef(Name), V, *Allocator);
		}
		void AddMember(const char* Name, float V)
		{
			JsonValue->AddMember(StringRef(Name), V, *Allocator);
		}
		void AddMember(const char* Name, const char* S)
		{
			Value StrValue;
			StrValue.SetString(S, (uint32)strlen(S), *Allocator);
			JsonValue->AddMember(StringRef(Name), StrValue, *Allocator);
		}
		void AddMember(const char* Name, const FFloat3& V)
		{
			Value VArray(kArrayType);
			VArray.Reserve(3, *Allocator);
			VArray.PushBack(V.X, *Allocator);
			VArray.PushBack(V.Y, *Allocator);
			VArray.PushBack(V.Z, *Allocator);
			JsonValue->AddMember(StringRef(Name), VArray, *Allocator);
		}
		void AddMember(const char* Name, const TVector<int32>& VI)
		{
			Value VArray(kArrayType);
			VArray.Reserve((SizeType)VI.size(), *Allocator);
			for (const auto& i : VI)
			{
				VArray.PushBack(i, *Allocator);
			}
			JsonValue->AddMember(StringRef(Name), VArray, *Allocator);
		}
		void AddMember(const char* Name, const TVector<uint32>& VI)
		{
			Value VArray(kArrayType);
			VArray.Reserve((SizeType)VI.size(), *Allocator);
			for (const auto& i : VI)
			{
				VArray.PushBack(i, *Allocator);
			}
			JsonValue->AddMember(StringRef(Name), VArray, *Allocator);
		}
		void AddMember(const char* Name, const TVector<float>& VF)
		{
			Value VArray(kArrayType);
			VArray.Reserve((SizeType)VF.size(), *Allocator);
			for (const auto& f : VF)
			{
				VArray.PushBack(f, *Allocator);
			}
			JsonValue->AddMember(StringRef(Name), VArray, *Allocator);
		}
		void AddMember(const char* Name, const int32* IData, int32 Count)
		{
			Value VArray(kArrayType);
			VArray.Reserve((SizeType)Count, *Allocator);
			for (int32 i = 0; i < Count; i++)
			{
				VArray.PushBack(IData[i], *Allocator);
			}
			JsonValue->AddMember(StringRef(Name), VArray, *Allocator);
		}
		void AddMember(const char* Name, const float* FData, int32 Count)
		{
			Value VArray(kArrayType);
			VArray.Reserve((SizeType)Count, *Allocator);
			for (int32 i = 0; i < Count; i++)
			{
				VArray.PushBack(FData[i], *Allocator);
			}
			JsonValue->AddMember(StringRef(Name), VArray, *Allocator);
		}
		void AddMember(const char* Name, const TVector<TString>& VS)
		{
			Value VArray(kArrayType);
			VArray.Reserve((SizeType)VS.size(), *Allocator);
			for (const auto& S : VS)
			{
				VArray.PushBack(StringRef(S.c_str()), *Allocator);
			}
			JsonValue->AddMember(StringRef(Name), VArray, *Allocator);
		}
		TJSONNode AddObject(const char* Name)
		{
			Value VObj(kObjectType);
			JsonValue->AddMember(Value::StringRefType(Name), VObj, *Allocator);
			return (*this)[Name];
		}
		TJSONNode AddArray(const char* Name)
		{
			Value VArray(kArrayType);
			JsonValue->AddMember(Value::StringRefType(Name), VArray, *Allocator);
			return (*this)[Name];
		}
		void Reserve(uint32 NewCap)
		{
			TI_ASSERT(JsonValue->IsArray());
			JsonValue->Reserve(NewCap, *Allocator);
		}
		TJSONNode InsertEmptyObjectToArray()
		{
			TI_ASSERT(JsonValue->IsArray());
			Value VObj(kObjectType);
			JsonValue->PushBack(VObj, *Allocator);
			return (*this)[JsonValue->Size() - 1];
		}
		TJSONNode InsertToArray(const FUInt3& V)
		{
			TI_ASSERT(JsonValue->IsArray());
			Value VArray(kArrayType);
			VArray.Reserve(3, *Allocator);
			VArray.PushBack(V.X, *Allocator);
			VArray.PushBack(V.Y, *Allocator);
			VArray.PushBack(V.Z, *Allocator);
			JsonValue->PushBack(VArray, *Allocator);
			return (*this)[JsonValue->Size() - 1];
		}
		template<class T>
		TJSONNode InsertToArray(const TVector<T>& V)
		{
			TI_ASSERT(JsonValue->IsArray());
			Value VArray(kArrayType);
			VArray.Reserve((uint32)V.size(), *Allocator);
			for (const T& v : V)
			{
				VArray.PushBack(v, *Allocator);
			}
			JsonValue->PushBack(VArray, *Allocator);
			return (*this)[JsonValue->Size() - 1];
		}

	protected:
		Value* JsonValue;
		TJSONAllocator* Allocator;
	};
	/////////////////////////////////////////////////////////////

	class TJSON : public TJSONNode
	{
	public:
		TJSON()
		{}

		virtual ~TJSON()
		{}

		void Parse(const int8* JsonText)
		{
			JsonDoc.Parse(JsonText);
			JsonValue = &JsonDoc;
		}

	protected:
		Document JsonDoc;
	};
	/////////////////////////////////////////////////////////////

	class TJSONWriter : public TJSONNode
	{
	public:
		TJSONWriter()
		{
			JsonDoc.SetObject();
			Allocator = &JsonDoc.GetAllocator();
			JsonValue = &JsonDoc;
		}

		virtual ~TJSONWriter()
		{}

		void Dump(TString& OutString)
		{
			//JsonDoc.AddMember("111", Root, *Allocator);
			StringBuffer Buffer;
			PrettyWriter<StringBuffer> Writer(Buffer);
			JsonDoc.Accept(Writer);
			OutString = Buffer.GetString();
		}

	protected:
		Document JsonDoc;
	};
}