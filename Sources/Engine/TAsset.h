/*
	TiX Engine v2.0 Copyright (C) 2018~2021
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	class TAssetLoader
	{
	public:
        TAssetLoader() {}
        virtual ~TAssetLoader() {}
		virtual void Load(TAssetPtr InAsset) = 0;
	};

	class TAssetLoaderSync : public TAssetLoader
	{
	public:
		virtual void Load(TAssetPtr InAsset) override;
	};

	class TAssetLoaderAync : public TAssetLoader
	{
	public:
		virtual void Load(TAssetPtr InAsset) override;
	};

	//////////////////////////////////////////////////////////////////
	enum E_ASSET_LOADING_STATE
	{
		ASSET_LOADING,
		ASSET_LOADED
	};
	class TAsset : public IReferenceCounted
	{
	public:
		TAsset(const TString& InName)
			: AssetName(InName)
			, LoadingState(ASSET_LOADING)
			, Loader(nullptr)
			, LoadingFinishDelegate(nullptr)
		{}

		~TAsset()
		{
			SAFE_DELETE(Loader);
			for (auto& Res : Resources)
			{
				Res = nullptr;
			}
			SAFE_DELETE(LoadingFinishDelegate);
		}

		void Load(bool bAync);

		void SetLoadingFinishDelegate(ILoadingFinishDelegate* InDelegate)
		{
			LoadingFinishDelegate = InDelegate;
		}

		TResource* GetResourcePtr(int32 Index = 0)
		{
			return Resources[Index].get();
		}

		void InitRenderThreadResource()
		{
			for (auto& Res : Resources)
			{
				Res->InitRenderThreadResource();
			}
		}

		void DestroyRenderThreadResource()
		{
			for (auto& Res : Resources)
			{
				Res->DestroyRenderThreadResource();
			}
		}

		void ClearResources()
		{
			for (auto& Res : Resources)
			{
				Res = nullptr;
			}
			Resources.clear();
		}

		bool HasReference() const
		{
			for (auto& Res : Resources)
			{
				if (Res->referenceCount() > 1)
				{
					return true;
				}
			}
			return false;
		}

		const TString& GetName() const
		{
			return AssetName;
		}

		const TVector<TResourcePtr>& GetResources() const
		{
			return Resources;
		}

		E_ASSET_LOADING_STATE GetLoadingState() const 
		{
			return LoadingState;
		}

		bool IsLoaded() const
		{
			return LoadingState != ASSET_LOADING;
		}
		
	private:
		void MarkAsLoaded()
		{
			LoadingState = ASSET_LOADED;
		}

	private:
		TString AssetName;
		E_ASSET_LOADING_STATE LoadingState;

		// Hold an source file for asynchronous loading
		TAssetLoader * Loader;
		ILoadingFinishDelegate * LoadingFinishDelegate;

		// Resource loaded
		TVector<TResourcePtr> Resources;

		friend class TAssetLoaderSync;
		friend class TAssetLoaderAync;
		friend class TAssetAyncLoadTask;
	};
}
