#pragma once

#include "../Component/MeshComponent.h"

struct BVHPrimitiveInfo
{
public:
	BVHPrimitiveInfo(size_t inPrimitiveIdx, TBoundingBox inBounds)
		:primitiveIndex(inPrimitiveIdx),
		bounds(inBounds),
		centroid(inBounds.GetCenter())
	{
	}

public:
	size_t primitiveIndex = 0;
	TBoundingBox bounds;
	TVector3 centroid = TVector3::Zero;
};

struct BVHBulidNode
{
public:
	void InitLeaf(int first, int count, const TBoundingBox& InBounds)
	{
		bounds = InBounds;
		firstPrimOffset = first;
		primitiveCount = count;
	}

	void InitInterior(int axis, std::unique_ptr<BVHBulidNode>&& left, std::unique_ptr<BVHBulidNode>&& right)
	{
		bounds = TBoundingBox::Union(left->bounds, right->bounds);
		splitAxis = axis;
		leftChild = std::move(left);
		rightChild = std::move(right);
	}

	bool IsLeafNode() { return primitiveCount > 0; }

public:
	TBoundingBox bounds;

	// For interior node
	int splitAxis = -1;

	std::unique_ptr<BVHBulidNode> leftChild = nullptr;
	std::unique_ptr<BVHBulidNode> rightChild = nullptr;

	// For leaf node
	int firstPrimOffset = -1;
	int primitiveCount = -1;
};

struct BVHBucketInfo
{
	int count = 0;
	TBoundingBox bounds;
};

struct BVHLinearNode
{
public:
	bool IsLeafNode() { return primitiveCount > 0; }

public:
	TBoundingBox bounds;

	// For interior node
	int splitAxis = -1;
	int secondChildOffset = -1;

	// For leaf node
	int firstPrimOffset = -1;
	int primitiveCount = -1;
};

class World;

class BVHAccelerator
{
public:
	enum class ESplitMethod
	{
		Middle,
		EqualCounts,
		SAH
	};

	BVHAccelerator(const std::vector<MeshComponent*>& meshComponents);

	void DebugBVHTree(World* world);
	void DebugFlattenBVH(World* world);

private:
	std::unique_ptr<BVHBulidNode> RecursiveBuild(std::vector<BVHPrimitiveInfo>& primitiveInfoList, int start, int end, int& outTotalNodes,
		std::vector<MeshComponent*>& orderedPrimitives);

	int PartitionMiddleMethod(const TBoundingBox& centroidBounds, int splitAxis, std::vector<BVHPrimitiveInfo>& primitiveInfoList,
		int start, int end);
	int PartitionEqualCountsMethod(const TBoundingBox& centroidBounds, int splitAxis, std::vector<BVHPrimitiveInfo>& primitiveInfoList,
		int start, int end);
	int PartitionSAHMethod(const TBoundingBox& bounds, const TBoundingBox& centroidBounds, int splitAxis, std::vector<BVHPrimitiveInfo>& primitiveInfoList,
		int start, int end, float& outCost);
	void CreateLeafNode(std::unique_ptr<BVHBulidNode>& outLeftNode, const TBoundingBox& bounds, std::vector<BVHPrimitiveInfo>& primitiveInfoList,
		int Start, int End, std::vector<MeshComponent*>& orderedPrimitives);
	int FlattenBVHTree(std::unique_ptr<BVHBulidNode>& node, int& offset);
	Color MapDepthToColor(int depth);
	void DebugBuildNode(World* world, BVHBulidNode* node, int depth);

private:
	ESplitMethod splitMethod = ESplitMethod::SAH;
	bool bTryAllAxisForSAH = true;
	const int maxPrimsInNode = 1;
	std::vector<MeshComponent*> cachePrimitives;
	std::unique_ptr<BVHBulidNode> rootNode = nullptr;
	std::vector<BVHLinearNode> linearNodes;
};