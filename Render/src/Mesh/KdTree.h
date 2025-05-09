#pragma once
#include <vector>
#include "BoundingBox.h"
#include "Primitive.h"
#include <memory>

enum KDNodeFlag
{
	SplitX,
	SplitY,
	SplitZ,
	Leaf
};

// TreeNode in building
struct KDTreeBulidNode
{
public:
	void InitLeaf(const std::vector<int>& indices)
	{
		flag = KDNodeFlag::Leaf;
		primitiveIndices = indices;
	}

	void InitInterior(KDNodeFlag splitAxis, const TBoundingBox& inBounds, float inSplitPos, std::unique_ptr<KDTreeBulidNode>&& below,
		std::unique_ptr<KDTreeBulidNode>&& above)
	{
		flag = splitAxis;
		bounds = inBounds;
		splitPos = inSplitPos;
		belowChild = std::move(below);
		aboveChild = std::move(above);
	}

	bool IsLeafNode() const
	{
		return flag == KDNodeFlag::Leaf;
	}

public:
	KDNodeFlag flag;

	// For interior node
	TBoundingBox bounds;

	float splitPos;

	std::unique_ptr<KDTreeBulidNode> belowChild = nullptr;
	std::unique_ptr<KDTreeBulidNode> aboveChild = nullptr;

	// For leaf node
	std::vector<int> primitiveIndices;
};

struct KDTreeBoundEdge
{
public:
	enum class EdgeType
	{
		Start,
		End
	};

	KDTreeBoundEdge(EdgeType inType, float inPos, int inPrimitiveIndex)
		:type(inType), pos(inPos), primitiveIndex(inPrimitiveIndex)
	{}

public:
	EdgeType type;
	float pos;
	int primitiveIndex;
};

// TreeNode after compressing
struct KDTreeLinearNode
{
public:
	bool IsLeafNode() const
	{
		return flag == KDNodeFlag::Leaf;
	}

public:
	KDNodeFlag flag;

	// For interior node
	TBoundingBox bounds;

	float splitPos;
	int aboveChildOffset = -1;

	// For leaf node
	std::vector<int> primitiveIndices;
};

struct KDTreeNodeToVisit
{
	KDTreeNodeToVisit(int Index, float Min, float Max)
		:nodeIndex(Index), tMin(Min), tMax(Max)
	{}

	int nodeIndex;
	float tMin, tMax;
};

class World;

class KDTreeAccelerator
{
public:
	KDTreeAccelerator(std::vector<std::shared_ptr<Primitive>> allPrimtives, int inMaxDepth = -1);

	void DebugKdTree(World* world) const;
	void DebugFlattenKdTree(World* world) const;
	bool Intersect(const Ray& ray, float& dist, bool& bBackFace) const;

	// Just for debug
	bool IntersectBruteForce(const Ray& ray, float& Dist, bool& bBackFace) const;

private:
	std::unique_ptr<KDTreeBulidNode> RecursiveBuild(const TBoundingBox& nodeBounds, const std::vector<int>& primitiveIndices,
		int depth, int& outTotalNodes);

	bool FindBestSplit(const TBoundingBox& nodeBounds, const std::vector<int>& primitiveIndices, int& SplitAxis, int& SplitOffset,
		std::vector<KDTreeBoundEdge>& splitEdges);

	int FlattenKdTree(std::unique_ptr<KDTreeBulidNode>& node, int& offset);

	Color MapDepthToColor(int depth) const;

	void DebugBuildNode(World* world, KDTreeBulidNode* node, int depth) const;

private:
	std::vector<std::shared_ptr<Primitive>> primitives;
	std::vector<TBoundingBox> primitiveBounds;
	std::unique_ptr<KDTreeBulidNode> rootNode = nullptr;
	std::vector<KDTreeLinearNode> linearNodes;
	const int maxPrimsInNode = 1;
	int maxDepth = -1;
	const float emptyBonus = 0.5f;
	const int traversalCost = 1;
	const int isectCost = 80;
};
