/*
	Copyright (C) 2012
	By Zhao Shuai tirax.cn@gmail.com 2012.7.18
*/

#include "stdafx.h"
#include "TNode.h"

namespace tix
{
	TNode::TNode(E_NODE_TYPE type, TNode* parent)
		: NodeType(type)
		, Parent(nullptr)
		, NodeFlag(ENF_VISIBLE | ENF_DIRTY_POS)
		, RelativeRotate(0.f, 0.f, 0.f, 1.f)
		, RelativeScale(1.f, 1.f, 1.f)
	{
		if (parent)
			parent->AddChild(this);
	}

	TNode::~TNode()
	{
		// Remove and delete all children
		Remove();
		RemoveAndDeleteAll();
	}

	void TNode::AddChild(TNode* child)
	{
		if (child && (child != this))
		{
			child->Remove(); // remove from old parent
			Children.push_back(child);
			child->Parent = this;
		}
	}

	void TNode::Remove()
	{
		if (Parent)
			Parent->RemoveChild(this);
	}

	bool TNode::RemoveChild(TNode* child)
	{
		VecRenderElements::iterator it = std::find(Children.begin(), Children.end(), child);
		if (it != Children.end())
		{
			(*it)->Parent = 0;
			Children.erase(it);
			return true;
		}
		return false;
	}

	void TNode::RemoveAndDeleteAll()
	{
		VecRenderElements::iterator it = Children.begin();
		for (; it != Children.end(); ++it)
		{
			(*it)->Parent = 0;
			ti_delete (*it);
		}
		Children.clear();
	}

	void TNode::SetPosition(const FFloat3& pos)
	{
		RelativePosition	= pos;
		NodeFlag		|= ENF_DIRTY_POS;
	}

	void TNode::SetScale(const FFloat3& scale)
	{
		RelativeScale		= scale;
		NodeFlag		|= ENF_DIRTY_SCALE;
	}

	void TNode::SetRotate(const FQuat& rotate)
	{
		RelativeRotate	= rotate;
		NodeFlag		|= ENF_DIRTY_ROT;
	}
	
	TNode* TNode::GetNodeById(const TString& uid)
	{
		if (NodeId == uid)
			return this;

		VecRenderElements::const_iterator it = Children.begin();
		for ( ; it != Children.end() ; ++ it)
		{
			TNode* node = (*it)->GetNodeById(uid);
			if (node)
				return node;
		}

		return NULL;
	}

	void TNode::GetNodesByType(E_NODE_TYPE type, TVector<TNode*>& nodes)
	{
		if ( NodeType == type)
		{
			nodes.push_back(this);
		}
		VecRenderElements::const_iterator it = Children.begin();
		for ( ; it != Children.end() ; ++ it)
		{
			(*it)->GetNodesByType(type, nodes);
		}
	}

	TNode* TNode::GetNodeByPath(const TString& NodePath)
	{
		TString Path = NodePath;
		TVector<TString> NodeIds;
		TString::size_type Start = 0;
		TString::size_type Pos;
		Pos = Path.find('.');
		TString S;
		while (Pos != TString::npos)
		{
			S = Path.substr(Start, Pos - Start);
			NodeIds.push_back(S);

			Start = Pos + 1;
			Pos = Path.find('.', Start);
		}
		S = Path.substr(Start);
		NodeIds.push_back(S);

		TNode* Current = this;
		for (uint32 i = 0; i < NodeIds.size(); ++i)
		{
			if (Current)
				Current = Current->GetNodeById(NodeIds[i]);
		}

		if (Current == this)
			return nullptr;
		else
			return Current;
	}

	TNode* TNode::IsIntersectWithRay(const FLine3& ray, FBox& outBBox, FFloat3& outIntersection)
	{
		// test children
		VecRenderElements::const_iterator it = Children.begin();
		for ( ; it != Children.end() ; ++ it)
		{
			TNode* node	= (*it)->IsIntersectWithRay(ray, outBBox, outIntersection);
			if (node)
			{
				return node;
			}
		}

		return NULL;
	}

	TNode* TNode::IsIntersectWithPoint(const FFloat3& p, FBox& outBBox, FFloat3& outIntersection)
	{
		// test children
		VecRenderElements::const_iterator it = Children.begin();
		for ( ; it != Children.end() ; ++ it)
		{
			TNode* node	= (*it)->IsIntersectWithPoint(p, outBBox, outIntersection);
			if (node)
			{
				return node;
			}
		}

		return NULL;
	}

	TNode* TNode::GetParent(E_NODE_TYPE type)
	{
		TNode* parent = GetParent();
		while (parent != nullptr)
		{
			if (parent->GetType() == type)
			{
				return parent;
			}
			parent = parent->GetParent();
		}
		return nullptr;
	}

	const FMat4& TNode::GetAbsoluteTransformation() const
	{
		return AbsoluteTransformation;
	}

	const FMat4& TNode::GetRelativeTransformation()
	{
		if ((NodeFlag & (ENF_DIRTY_TRANSFORM)) != 0)
		{
			if (NodeFlag & (ENF_DIRTY_SCALE | ENF_DIRTY_ROT))
			{
				RelativeRotate.GetMatrix(RelativeTransformation);
				if (RelativeScale != FFloat3(1.f, 1.f, 1.f))
				{
					RelativeTransformation.PostScale(RelativeScale);
				}
				RelativeTransformation.SetTranslation(RelativePosition);
			}
			else
			{
				RelativeTransformation.SetTranslation(RelativePosition);
			}
			NodeFlag &= ~ENF_DIRTY_TRANSFORM;
		}
		return RelativeTransformation;
	}

	void TNode::Tick(float Dt)
	{
		for (auto* Child : Children)
		{
			Child->Tick(Dt);
		}
	}

	void TNode::UpdateAllTransformation()
	{
		UpdateAbsoluteTransformation();

		VecRenderElements::const_iterator it = Children.begin();
		for (; it != Children.end(); ++it)
		{
			(*it)->UpdateAllTransformation();
		}
	}

	void TNode::UpdateAbsoluteTransformation()
	{
		// clear ENF_ABSOLUTETRANSFORMATION_UPDATED at the begin of this frame
		NodeFlag &= ~ENF_ABSOLUTETRANSFORMATION_UPDATED;

		// update absolute transformation if need. And mark ENF_ABSOLUTETRANSFORMATION_UPDATED if transformation is updated.
		if (Parent &&
			((Parent->NodeFlag & ENF_ABSOLUTETRANSFORMATION_UPDATED) ||
			(NodeFlag & ENF_DIRTY_TRANSFORM)))
		{
			Parent->GetAbsoluteTransformation().Mult34(GetRelativeTransformation(), AbsoluteTransformation);
			NodeFlag |= ENF_ABSOLUTETRANSFORMATION_UPDATED;
		}
		else
		{
			if (NodeFlag & ENF_DIRTY_TRANSFORM)
			{
				AbsoluteTransformation = GetRelativeTransformation();
				NodeFlag |= ENF_ABSOLUTETRANSFORMATION_UPDATED;
			}
		}
	}
}
