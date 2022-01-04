/*
	TiX Engine v2.0 Copyright (C) 2018~2021
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	class TNode;

	class TNodeFactory
	{
	public:
		template<class T>
		static T* CreateNode(TNode* Parent, const TString& NodeId = TString())
		{
			T* Node = ti_new T(Parent);
			Node->SetId(NodeId);
			return Node;
		}
	};
	
#define DECLARE_NODE_WITH_CONSTRUCTOR(NodeName) \
		friend class TNodeFactory; \
	protected: \
		TNode##NodeName(TNode * Parent); \
	public: \
		static const E_NODE_TYPE NODE_TYPE = ENT_##NodeName;

	class TNode 
	{
		friend class TNodeFactory;
	public:
		static const E_NODE_TYPE NODE_TYPE = ENT_Node;
	protected:
		TNode(E_NODE_TYPE type, TNode* parent);
		virtual ~TNode();
	public:

		virtual const TString& GetId() const
		{
			return NodeId;
		}
		virtual void SetId(const TString& InNodeId)
		{
			NodeId = InNodeId;
		}

		virtual void AddChild(TNode* child);		// add child at the end of children
		virtual void Remove();
		
		virtual void SetPosition(const vector3df& pos);
		virtual void SetScale(const vector3df& scale);
		virtual void SetRotate(const quaternion& rotate);
		
		virtual TNode* GetNodeById(const TString& uid);
		virtual void GetNodesByType(E_NODE_TYPE type, TVector<TNode*>& elements);
		virtual TNode* GetNodeByPath(const TString& NodePath);
		
		virtual TNode* IsIntersectWithRay(const line3df& ray, aabbox3df& outBBox, vector3df& outIntersection);
		virtual TNode* IsIntersectWithPoint(const vector3df& p, aabbox3df& outBBox, vector3df& outIntersection);

		virtual void Tick(float Dt);

		// Update all note's transformation in game thread, since some tick need that
		virtual void UpdateAllTransformation();
		virtual const matrix4& GetAbsoluteTransformation() const;

		TNode* GetParent()
		{
			return Parent;
		}
		TI_API TNode* GetParent(E_NODE_TYPE type);

		TNode* GetChild(uint32 index)
		{
			TI_ASSERT(index < Children.size());
			return Children[index];
		}

		E_NODE_TYPE GetType() const
		{
			return NodeType;
		}

		void SetFlag(E_NODE_FLAG Flag, bool enable)
		{
			if (enable)
				NodeFlag |= Flag;
			else
				NodeFlag &= ~Flag;
		}

		bool HasFlag(E_NODE_FLAG Flag) const
		{
			return (NodeFlag & Flag) != 0;
		}

		uint32	GetChildrenCount()
		{
			return (uint32)Children.size();
		}

		virtual const vector3df& GetRelativePosition() const
		{
			return RelativePosition;
		}

		virtual const quaternion& GetRelativeRotation() const
		{
			return RelativeRotate;
		}

		virtual const vector3df& GetRelativeScale() const
		{
			return RelativeScale;
		}

		virtual vector3df GetAbsolutePosition()
		{
			return AbsoluteTransformation.getTranslation();
		}

		inline bool IsVisible()
		{
			return (NodeFlag & ENF_VISIBLE) != 0;
		}
		inline void SetVisible(bool visible)
		{
			if (visible)
				NodeFlag |= ENF_VISIBLE;
			else
				NodeFlag &= ~ENF_VISIBLE;
		}
		
		virtual void RemoveAndDeleteAll();

		virtual void BindLights(TVector<TNode*>& Lights, bool ForceRebind) {};

	protected:
		virtual bool RemoveChild(TNode* child);
		virtual void UpdateAbsoluteTransformation();
		virtual const matrix4& GetRelativeTransformation();

	protected:
		union {
			E_NODE_TYPE NodeType;
			char NodeTypeName[4];
		};
		TString NodeId;

		TNode * Parent;
		typedef TVector<TNode*>	VecRenderElements;
		VecRenderElements Children;

		uint32 NodeFlag;

		vector3df RelativePosition;
		quaternion RelativeRotate;
		vector3df RelativeScale;

		matrix4 AbsoluteTransformation;
		matrix4 RelativeTransformation;
	};

} // end namespace tix

