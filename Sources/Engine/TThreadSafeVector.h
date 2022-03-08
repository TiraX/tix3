/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	template <class T>
	class TThreadSafeVector
	{
	public:
		TThreadSafeVector()
		{}

		~TThreadSafeVector()
		{}

		uint32 Size()
		{
			return (uint32)V.size();
		}
		void Reserve(uint32 NewCap)
		{
			Mutex.lock();
			V.reserve(NewCap);
			Mutex.unlock();
		}
		void Resize(uint32 NewCap)
		{
			Mutex.lock();
			V.resize(NewCap);
			Mutex.unlock();
		}
		void PushBack(const T& o)
		{
			Mutex.lock();
			V.push_back(o);
			Mutex.unlock();
		}
		void PopBack()
		{
			Mutex.lock();
			V.pop_back();
			Mutex.unlock();
		}
		void Clear()
		{
			Mutex.lock();
			V.clear();
			Mutex.unlock();
		}
		void Lock()
		{
			Mutex.lock();
		}
		void Unlock()
		{
			Mutex.unlock();
		}
		T operator[](const uint32 _Pos) const
		{
			Mutex.lock();
			T R = V[_Pos];
			Mutex.unlock();
			return R;
		}

	private:
		mutable TMutex Mutex;
		TVector<T> V;
	};
}