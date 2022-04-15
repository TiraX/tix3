/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

typedef unsigned char uint8;
typedef char int8;

typedef unsigned short uint16;
typedef short int16;

typedef unsigned int uint32;
typedef int int32;

#if defined (TI_PLATFORM_WIN32)
typedef __int64 int64;
typedef unsigned __int64 uint64;
#else
typedef unsigned long long uint64;
typedef long long int64;
#endif

#include "Math/half.hpp"
using namespace half_float;
typedef half float16;

typedef float float32;
typedef double float64;

#define TVector std::vector
#define TList std::list
#define TPair std::pair
#define TMap std::map
#define THMap std::unordered_map
#define TSet std::set
#define THSet std::unordered_set
#define TNumLimit std::numeric_limits
#define TFunction std::function
#define TForward std::forward
#define TMakePair std::make_pair
#define TIsDigit std::isdigit

// Release memory of TVector
template<class T>
inline void ReleaseTVector(TVector<T>& V)
{
	TVector<T>().swap(V);
}

typedef std::string TString;
typedef std::wstring TWString;
typedef std::stringstream TStringStream;

typedef std::mutex TMutex;
typedef std::condition_variable TCond;
typedef std::thread::id TThreadId;

#include "TInputEventType.h"
#include "TTypeCull.h"
#include "TTypeRenderer.h"
#include "TTypeRenderResource.h"
#include "TTypeNode.h"
#include "TTypePixelFormat.h"
#include "TTypeScene.h"

#include "SColor.h"
//#include <TiUString.h>

