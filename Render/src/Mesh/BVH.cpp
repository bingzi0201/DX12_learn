#include "BVH.h"
#include <algorithm>
#include <stack>
#include "../World/World.h"

BVHAccelerator::BVHAccelerator(const std::vector<MeshComponent*>& meshComponents)
{
	// Initialize PrimitiveInfo list
	std::vector<BVHPrimitiveInfo> primitiveInfoList;
	for (size_t i = 0; i < meshComponents.size(); i++)
	{
		TBoundingBox worldBound;
		if (meshComponents[i]->GetWorldBoundingBox(worldBound))
		{
			primitiveInfoList.push_back({ i, worldBound });
			cachePrimitives.push_back(meshComponents[i]);
		}
	}

	// Build BVH tree
	std::vector<MeshComponent*> orderedPrimitives;
	orderedPrimitives.reserve(primitiveInfoList.size());

	int totalNodes = 0;
	rootNode = RecursiveBuild(primitiveInfoList, 0, (int)primitiveInfoList.size(), totalNodes, orderedPrimitives);

	cachePrimitives.swap(orderedPrimitives);

	// Compute representation of depth-first traversal of BVH tree
	linearNodes.resize(totalNodes);
	int Offset = 0;
	FlattenBVHTree(rootNode, Offset);
}

std::unique_ptr<BVHBulidNode> BVHAccelerator::RecursiveBuild(std::vector<BVHPrimitiveInfo>& primitiveInfoList, int start, int end, int& outTotalNodes,
	std::vector<MeshComponent*>& orderedPrimitives)
{
	assert(start < end);

	std::unique_ptr<BVHBulidNode> node = std::make_unique<BVHBulidNode>();

	outTotalNodes++;

	// Compute bounds of all primitives in BVH node
	TBoundingBox bounds;
	for (int i = start; i < end; i++)
	{
		bounds = TBoundingBox::Union(bounds, primitiveInfoList[i].bounds);
	}

	int primitiveCount = end - start;
	if (primitiveCount == 1) // Create leaf node
	{
		CreateLeafNode(node, bounds, primitiveInfoList, start, end, orderedPrimitives);

		return node;
	}
	else
	{
		// Compute bound of primitive centroids, choose split axis
		TBoundingBox centroidBounds;
		for (int i = start; i < end; i++)
		{
			centroidBounds = TBoundingBox::Union(centroidBounds, primitiveInfoList[i].bounds);
		}
		int splitAxis = centroidBounds.GetWidestAxis();

		// If the width of widest axis equal to 0, then we can't split primitives to two sets
		// so create leaf node
		if (centroidBounds.boxMax[splitAxis] == centroidBounds.boxMin[splitAxis])
		{
			CreateLeafNode(node, bounds, primitiveInfoList, start, end, orderedPrimitives);
			return node;
		}

		// Partition primitives into two sets and build children
		int mid = (start + end) / 2;
		switch (splitMethod)
		{
		case BVHAccelerator::ESplitMethod::Middle:
		{
			mid = PartitionMiddleMethod(centroidBounds, splitAxis, primitiveInfoList, start, end);

			if (mid == start || mid == end) // Partition fail, use EqualCounts as an alternative
			{
				mid = PartitionEqualCountsMethod(centroidBounds, splitAxis, primitiveInfoList, start, end);
			}

			break;
		}
		case BVHAccelerator::ESplitMethod::EqualCounts:
		{
			mid = PartitionEqualCountsMethod(centroidBounds, splitAxis, primitiveInfoList, start, end);

			break;
		}
		case ESplitMethod::SAH:
		{
			if (primitiveCount <= 2)
			{
				mid = PartitionEqualCountsMethod(centroidBounds, splitAxis, primitiveInfoList, start, end);
			}
			else
			{
				if (bTryAllAxisForSAH) // Try all axis to find the best partition
				{
					float minCost = TMath::Infinity;
					int bestAxis = -1, bestMid = -1;
					for (int curAxis = 0; curAxis < 3; curAxis++) // Try all axis, compute their cost
					{
						float CurCost = 0.0f;
						int CurMid = PartitionSAHMethod(bounds, centroidBounds, curAxis, primitiveInfoList, start, end, CurCost);

						if (CurMid != -1 && CurCost < minCost)
						{
							minCost = CurCost;
							bestAxis = curAxis;
							bestMid = CurMid;
						}
					}

					if (bestAxis != -1) // Use the least costly axis we found
					{
						splitAxis = bestAxis;
						mid = bestMid;
					}
					else
					{
						mid = -1;
					}
				}
				else // Only consider the widest axis
				{
					float Cost = 0.0f;
					mid = PartitionSAHMethod(bounds, centroidBounds, splitAxis, primitiveInfoList, start, end, Cost);
				}


				if (mid == -1) // Create leaf node
				{
					CreateLeafNode(node, bounds, primitiveInfoList, start, end, orderedPrimitives);

					return node;
				}

				if (mid == start || mid == end) // Partition fail, use EqualCounts as an alternative
				{
					mid = PartitionEqualCountsMethod(centroidBounds, splitAxis, primitiveInfoList, start, end);
				}
			}

			break;
		}
		default:
			break;
		}

		node->InitInterior(splitAxis,
			RecursiveBuild(primitiveInfoList, start, mid,
				outTotalNodes, orderedPrimitives),
			RecursiveBuild(primitiveInfoList, mid, end,
				outTotalNodes, orderedPrimitives));

		return node;
	}
}

int BVHAccelerator::PartitionMiddleMethod(const TBoundingBox& centroidBounds, int splitAxis,
	std::vector<BVHPrimitiveInfo>& primitiveInfoList, int start, int end)
{
	// Partition primitives through node's midpoint
	float AxisMid = (centroidBounds.boxMin[splitAxis] + centroidBounds.boxMax[splitAxis]) / 2;

	BVHPrimitiveInfo* MidPtr = std::partition(
		&primitiveInfoList[start], &primitiveInfoList[end - 1] + 1,
		[splitAxis, AxisMid](const BVHPrimitiveInfo& PrimitiveInfo)
		{
			return PrimitiveInfo.centroid[splitAxis] < AxisMid;
		});

	int Mid = int(MidPtr - &primitiveInfoList[0]);

	return Mid;
}

int BVHAccelerator::PartitionEqualCountsMethod(const TBoundingBox& centroidBounds, int splitAxis,
	std::vector<BVHPrimitiveInfo>& primitiveInfoList, int start, int end)
{
	// Partition primitives into equally-sized subsets
	int mid = (start + end) / 2;
	std::nth_element(&primitiveInfoList[start], &primitiveInfoList[mid],
		&primitiveInfoList[end - 1] + 1,
		[splitAxis](const BVHPrimitiveInfo& a, const BVHPrimitiveInfo& b)
		{
			return a.centroid[splitAxis] < b.centroid[splitAxis];
		});

	return mid;
}

int ComputeSAHBucketIndex(int bucketCount, const TBoundingBox& centroidBounds, int splitAxis, const BVHPrimitiveInfo& primitiveInfo)
{
	float AxisWidth = centroidBounds.boxMax[splitAxis] - centroidBounds.boxMin[splitAxis];
	float Offset = primitiveInfo.centroid[splitAxis] - centroidBounds.boxMin[splitAxis];

	int BucketIdx = int(bucketCount * (Offset / AxisWidth));
	if (BucketIdx == bucketCount)
	{
		BucketIdx = bucketCount - 1;
	}
	assert(BucketIdx >= 0);
	assert(BucketIdx < bucketCount);

	return BucketIdx;
}

int BVHAccelerator::PartitionSAHMethod(const TBoundingBox& bounds, const TBoundingBox& centroidBounds, int splitAxis,
	std::vector<BVHPrimitiveInfo>& primitiveInfoList, int start, int end, float& outCost)
{
	// Allocate BucketInfos for SAH partition buckets
	const int BucketCount = 12;
	BVHBucketInfo Buckets[BucketCount];

	// Initialize BucketInfos for SAH partition buckets
	for (int i = start; i < end; ++i)
	{
		int BucketIdx = ComputeSAHBucketIndex(BucketCount, centroidBounds, splitAxis, primitiveInfoList[i]);

		Buckets[BucketIdx].count++;
		Buckets[BucketIdx].bounds = TBoundingBox::Union(Buckets[BucketIdx].bounds, primitiveInfoList[i].bounds);
	}

	// Compute costs for splitting after each bucket
	float Cost[BucketCount - 1];
	for (int i = 0; i < BucketCount - 1; ++i)
	{
		TBoundingBox Box0, Box1;
		int Count0 = 0, Count1 = 0;
		for (int j = 0; j <= i; ++j)
		{
			Box0 = TBoundingBox::Union(Box0, Buckets[j].bounds);
			Count0 += Buckets[j].count;
		}
		for (int j = i + 1; j < BucketCount; ++j)
		{
			Box1 = TBoundingBox::Union(Box1, Buckets[j].bounds);
			Count1 += Buckets[j].count;
		}

		Cost[i] = 1.0f + (Count0 * Box0.GetSurfaceArea() + Count1 * Box1.GetSurfaceArea()) / bounds.GetSurfaceArea();
	}

	// Find bucket to split at that minimizes SAH metric
	float MinCost = Cost[0];
	int MinCostSplitBucket = 0;
	for (int i = 1; i < BucketCount - 1; ++i)
	{
		if (Cost[i] < MinCost)
		{
			MinCost = Cost[i];
			MinCostSplitBucket = i;
		}
	}

	outCost = MinCost;

	// Either split primitives at selected SAH bucket or create leaf
	int PrimitiveCount = end - start;
	float LeafCost = float(PrimitiveCount);
	if (PrimitiveCount > maxPrimsInNode || MinCost < LeafCost)
	{
		BVHPrimitiveInfo* MidPtr = std::partition(
			&primitiveInfoList[start], &primitiveInfoList[end - 1] + 1,
			[=](const BVHPrimitiveInfo& PrimitiveInfo)
			{
				int BucketIdx = ComputeSAHBucketIndex(BucketCount, centroidBounds, splitAxis, PrimitiveInfo);

				return BucketIdx <= MinCostSplitBucket;
			});

		int Mid = int(MidPtr - &primitiveInfoList[0]);
		return Mid;
	}
	else
	{
		// Create leaf node

		return -1;
	}
}

void BVHAccelerator::CreateLeafNode(std::unique_ptr<BVHBulidNode>& outLeftNode, const TBoundingBox& bounds,
	std::vector<BVHPrimitiveInfo>& primitiveInfoList, int start, int end, std::vector<MeshComponent*>& orderedPrimitives)
{
	int FirstPrimOffset = (int)orderedPrimitives.size();
	for (int i = start; i < end; ++i)
	{
		size_t PrimitiveIdx = primitiveInfoList[i].primitiveIndex;
		orderedPrimitives.push_back(cachePrimitives[PrimitiveIdx]);
	}

	int PrimitiveCount = end - start;
	outLeftNode->InitLeaf(FirstPrimOffset, PrimitiveCount, bounds);
}

int BVHAccelerator::FlattenBVHTree(std::unique_ptr<BVHBulidNode>& node, int& offset)
{
	BVHLinearNode& LinearNode = linearNodes[offset];
	int MyOffset = offset;
	offset++;

	LinearNode.bounds = node->bounds;
	if (node->IsLeafNode()) // Left node
	{
		assert(node->leftChild == nullptr);
		assert(node->rightChild == nullptr);

		LinearNode.firstPrimOffset = node->firstPrimOffset;
		LinearNode.primitiveCount = node->primitiveCount;
	}
	else // Interior node
	{
		LinearNode.splitAxis = node->splitAxis;

		FlattenBVHTree(node->leftChild, offset);

		LinearNode.secondChildOffset = FlattenBVHTree(node->rightChild, offset);
	}

	return MyOffset;
}

void BVHAccelerator::DebugBVHTree(World* world)
{
	DebugBuildNode(world, rootNode.get(), 0);
}

Color BVHAccelerator::MapDepthToColor(int depth)
{
	Color color;
	switch (depth)
	{
	case 0:
		color = Color::Red;
		break;
	case 1:
		color = Color::Yellow;
		break;
	case 2:
		color = Color::Green;
		break;
	case 3:
		color = Color::Blue;
		break;
	default:
		color = Color::White;
		break;
	}

	return color;
}

void BVHAccelerator::DebugBuildNode(World* world, BVHBulidNode* node, int depth)
{
	if (node == nullptr)
	{
		return;
	}

	Color color = MapDepthToColor(depth);

	TVector3 Offset = TVector3(0.1f) * float(std::clamp(5 - depth, 0, 5));

	world->DrawBox3D(node->bounds.boxMin - Offset, node->bounds.boxMax + Offset, color);

	DebugBuildNode(world, node->leftChild.get(), depth + 1);
	DebugBuildNode(world, node->rightChild.get(), depth + 1);
}

void BVHAccelerator::DebugFlattenBVH(World* world)
{
	if (linearNodes.empty())
	{
		return;
	}

	int CurrentVisitNodeIdx = 0;
	std::stack<int> NodesToVisit;

	while (true)
	{
		BVHLinearNode& Node = linearNodes[CurrentVisitNodeIdx];

		if (Node.IsLeafNode()) // Leaf node
		{
			world->DrawBox3D(Node.bounds.boxMin, Node.bounds.boxMax, Color::White);

			if (NodesToVisit.empty())
			{
				break;
			}
			else
			{
				CurrentVisitNodeIdx = NodesToVisit.top();
				NodesToVisit.pop();
			}
		}
		else // Interior node
		{
			world->DrawBox3D(Node.bounds.boxMin, Node.bounds.boxMax, Color::Red);

			NodesToVisit.push(Node.secondChildOffset);
			CurrentVisitNodeIdx++;
		}
	}
}