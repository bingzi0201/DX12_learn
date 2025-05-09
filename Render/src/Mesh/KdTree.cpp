#include "KdTree.h"
#include "../World/World.h"
#include <algorithm>
#include <stack>
#include <cmath>

KDTreeAccelerator::KDTreeAccelerator(std::vector<std::shared_ptr<Primitive>> allPrimtives, int inMaxDepth)
{
	// Initial Primitive and bounds
	TBoundingBox rootBounds;
	for (size_t i = 0; i < allPrimtives.size(); i++)
	{
		TBoundingBox worldBound;
		if (allPrimtives[i]->GetWorldBoundingBox(worldBound))
		{
			primitives.push_back(allPrimtives[i]);
			primitiveBounds.push_back(worldBound);

			rootBounds = TBoundingBox::Union(rootBounds, worldBound);
		}
	}

	maxDepth = inMaxDepth;
	if (maxDepth <= 0)
	{
		maxDepth = (int)std::round(8 + 1.3f * TMath::Log2Int(int64_t(primitives.size())));
	}

	// Init PrimitiveIndices for root node
	std::vector<int> rootPrimitiveIndices;
	rootPrimitiveIndices.reserve(primitives.size());
	for (int i = 0; i < primitives.size(); i++)
	{
		rootPrimitiveIndices.push_back(i);
	}

	// Recursive bulid kd-tree
	int totalNodes = 0;
	rootNode = RecursiveBuild(rootBounds, rootPrimitiveIndices, 0, totalNodes);

	// Compute representation of depth-first traversal of kd tree
	linearNodes.resize(totalNodes);
	int offset = 0;
	FlattenKdTree(rootNode, offset);
}


std::unique_ptr<KDTreeBulidNode> KDTreeAccelerator::RecursiveBuild(const TBoundingBox& nodeBounds, const std::vector<int>& primitiveIndices,
	int depth, int& outTotalNodes)
{
	std::unique_ptr<KDTreeBulidNode> node = std::make_unique<KDTreeBulidNode>();
	outTotalNodes++;

	int primitiveCount = int(primitiveIndices.size());
	if (primitiveCount <= maxPrimsInNode || depth == maxDepth) // Create leaf node
	{
		node->InitLeaf(primitiveIndices);

		return node;
	} 
	else
	{
		// Find the best splitAxis and split position
		int splitAxis = -1;
		int splitOffset = -1;
		std::vector<KDTreeBoundEdge> splitEdges;
		bool bSuccess = FindBestSplit(nodeBounds, primitiveIndices, splitAxis, splitOffset, splitEdges);

		if (!bSuccess) //Create leaf node
		{
			node->InitLeaf(primitiveIndices);
			return node;
		}

		// Classify primitives with respect to split
		std::vector<int> belowPrimitiveIndices;
		std::vector<int> abovePrimitiveIndices;
		for (int i = 0; i < splitOffset; i++)
		{
			if (splitEdges[i].type == KDTreeBoundEdge::EdgeType::Start)
			{
				belowPrimitiveIndices.push_back(splitEdges[i].primitiveIndex);
			}
		}
		for (int i = splitOffset + 1; i < splitEdges.size(); i++)
		{
			if (splitEdges[i].type == KDTreeBoundEdge::EdgeType::End)
			{
				abovePrimitiveIndices.push_back(splitEdges[i].primitiveIndex);
			}
		}

		// Recursively initialize children nodes
		float splitPos = splitEdges[splitOffset].pos;
		TBoundingBox belowBoundingBox = nodeBounds;
		belowBoundingBox.boxMax[splitAxis] = splitPos;
		TBoundingBox aboveBoundingBox = nodeBounds;
		aboveBoundingBox.boxMin[splitAxis] = splitPos;

		node->InitInterior(KDNodeFlag(splitAxis), nodeBounds, splitPos,
			RecursiveBuild(belowBoundingBox, belowPrimitiveIndices, depth + 1, outTotalNodes),
			RecursiveBuild(aboveBoundingBox, abovePrimitiveIndices, depth + 1, outTotalNodes));
	}

	return node;
}

bool KDTreeAccelerator::FindBestSplit(const TBoundingBox& nodeBounds, const std::vector<int>& primitiveIndices, int& splitAxis,
	int& splitOffset, std::vector<KDTreeBoundEdge>& splitEdges)
{
	splitAxis = -1;
	splitOffset = -1;

	int primitiveCount = int(primitiveIndices.size());

	TVector3 totalSize = nodeBounds.GetSize();
	float TotalSA = nodeBounds.GetSurfaceArea();
	float InvTotalSA = 1.0f / TotalSA;
	float bestCost = TMath::Infinity;

	for (int axis = 0; axis < 3; axis++)
	{
		int oldSplitAixs = splitAxis;

		// Initialize edges for axis
		std::vector<KDTreeBoundEdge> edges;
		for (int index : primitiveIndices)
		{
			const TBoundingBox& bound = primitiveBounds[index];

			// Add start and end edge for this primitive
			edges.push_back(KDTreeBoundEdge(KDTreeBoundEdge::EdgeType::Start, bound.boxMin[axis], index));
			edges.push_back(KDTreeBoundEdge(KDTreeBoundEdge::EdgeType::End, bound.boxMax[axis], index));
		}

		// Sort edges for axis
		std::sort(edges.begin(), edges.end(),
			[](const KDTreeBoundEdge& edge0, const KDTreeBoundEdge& edge1) -> bool
			{
				if (edge0.pos == edge1.pos)
					return (int)edge0.type < (int)edge1.type;
				else
					return edge0.pos < edge1.pos;
			});

		// Compute cost of all splits for axis to find best
		int belowCount = 0, aboveCount = primitiveCount;
		for (int edgeIndex = 0; edgeIndex < edges.size(); edgeIndex++)
		{
			KDTreeBoundEdge& edge = edges[edgeIndex];

			if (edge.type == KDTreeBoundEdge::EdgeType::End)
			{
				aboveCount--;
			}

			float edgePos = edge.pos;
			if (edgePos > nodeBounds.boxMin[axis] && edgePos < nodeBounds.boxMax[axis])
			{
				int otherAxis0 = (axis + 1) % 3;
				int otherAxis1 = (axis + 2) % 3;

				float belowSA = 2 * (totalSize[otherAxis0] * totalSize[otherAxis1] +
					(edgePos - nodeBounds.boxMin[axis]) *
					(totalSize[otherAxis0] + totalSize[otherAxis1]));
				float AboveSA = 2 * (totalSize[otherAxis0] * totalSize[otherAxis1] +
					(nodeBounds.boxMax[axis] - edgePos) *
					(totalSize[otherAxis0] + totalSize[otherAxis1]));

				float pBelow = belowSA * InvTotalSA;
				float pAbove = AboveSA * InvTotalSA;
				float bonus = (aboveCount == 0 || belowCount == 0) ? emptyBonus : 0;
				float cost = traversalCost + isectCost * (1 - bonus) * (pBelow * belowCount + pAbove * aboveCount);

				// Update best split if this is lowest cost so far
				if (cost < bestCost)
				{
					bestCost = cost;
					splitAxis = axis;
					splitOffset = edgeIndex;
				}
			}

			if (edge.type == KDTreeBoundEdge::EdgeType::Start)
			{
				belowCount++;
			}
		}
		assert(belowCount == primitiveCount && aboveCount == 0);

		if (splitAxis != oldSplitAixs)
		{
			splitEdges = edges;
		}
	}

	//TODO: BAD REFINE

	if ((bestCost > 4 * isectCost * primitiveCount && primitiveCount < 16) || splitAxis == -1) // Create leaf node
	{
		return false;
	}

	return true;
}

int KDTreeAccelerator::FlattenKdTree(std::unique_ptr<KDTreeBulidNode>& node, int& offset)
{
	KDTreeLinearNode& linearNode = linearNodes[offset];
	int myOffset = offset;
	offset++;

	linearNode.flag = node->flag;
	if (node->IsLeafNode()) // Left node
	{
		assert(node->belowChild == nullptr);
		assert(node->aboveChild == nullptr);

		linearNode.primitiveIndices = node->primitiveIndices;
	}
	else // Interior node
	{
		linearNode.bounds = node->bounds;
		linearNode.splitPos = node->splitPos;
		FlattenKdTree(node->belowChild, offset);
		linearNode.aboveChildOffset = FlattenKdTree(node->aboveChild, offset);
	}

	return myOffset;
}

void KDTreeAccelerator::DebugKdTree(World* World) const
{
	DebugBuildNode(World, rootNode.get(), 0);
}

Color KDTreeAccelerator::MapDepthToColor(int Depth) const
{
	Color color;
	switch (Depth)
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
		color = Color::Cyan;
		break;
	case 4:
		color = Color::Blue;
		break;
	default:
		color = Color::Magenta;
		break;
	}

	return color;
}

void KDTreeAccelerator::DebugBuildNode(World* world, KDTreeBulidNode* Node, int Depth) const
{
	if (Node == nullptr)
	{
		return;
	}

	Color color = MapDepthToColor(Depth);

	TVector3 Offset = TVector3(0.05f) * float(std::clamp(5 - Depth, 0, 5));
	//TVector3 Offset = TVector3::Zero;

	if (Node->IsLeafNode())
	{
		for (int Index : Node->primitiveIndices)
		{
			const TBoundingBox& bounds = primitiveBounds[Index];

			world->DrawBox3D(bounds.boxMin - Offset, bounds.boxMax + Offset, Color::White);
		}
	}
	else
	{
		world->DrawBox3D(Node->bounds.boxMin - Offset, Node->bounds.boxMax + Offset, color);

		DebugBuildNode(world, Node->belowChild.get(), Depth + 1);
		DebugBuildNode(world, Node->aboveChild.get(), Depth + 1);
	}
}

void KDTreeAccelerator::DebugFlattenKdTree(World* world) const
{
	if (linearNodes.empty())
	{
		return;
	}

	int CurrentVisitNodeIdx = 0;
	std::stack<int> NodesToVisit;

	while (true)
	{
		const KDTreeLinearNode& Node = linearNodes[CurrentVisitNodeIdx];

		if (Node.IsLeafNode()) // Leaf node
		{
			for (int Index : Node.primitiveIndices)
			{
				const TBoundingBox& bounds = primitiveBounds[Index];

				world->DrawBox3D(bounds.boxMin, bounds.boxMax, Color::White);
			}

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

			NodesToVisit.push(Node.aboveChildOffset);
			CurrentVisitNodeIdx++;
		}
	}
}

bool KDTreeAccelerator::Intersect(const Ray& ray, float& dist, bool& bBackFace) const
{
	if (linearNodes.empty())
	{
		return false;
	}

	// Compute initial parametric range of ray inside kd-tree extent
	float tMin, tMax;
	TBoundingBox rootBounds = linearNodes[0].bounds;
	if (!rootBounds.Intersect(ray, tMin, tMax))
	{
		return false;
	}

	// Traverse kd-tree nodes in order for ray
	bool bHit = false;
	TVector3 invDir(1.0f / ray.direction.x, 1.0f / ray.direction.y, 1.0f / ray.direction.z);

	int currentNodeIdx = 0;
	std::stack<KDTreeNodeToVisit> nodesToVisit;

	while (true)
	{
		if (ray.maxDist < tMin)
		{
			break;
		}

		const KDTreeLinearNode& node = linearNodes[currentNodeIdx];

		if (node.IsLeafNode()) // Leaf node
		{
			for (int Index : node.primitiveIndices)
			{
				auto primitive = primitives[Index];
				if (primitive->Intersect(ray, dist, bBackFace))
				{
					bHit = true;
					ray.maxDist = dist;
				}
			}

			if (nodesToVisit.empty())
			{
				break;
			}
			else
			{
				KDTreeNodeToVisit nodeToVisit = nodesToVisit.top();
				nodesToVisit.pop();

				currentNodeIdx = nodeToVisit.nodeIndex;
				tMin = nodeToVisit.tMin;
				tMax = nodeToVisit.tMax;
			}
		}
		else // Interior node
		{
			// Compute parametric distance along ray to split plane
			int axis = int(node.flag);
			float tPlane = (node.splitPos - ray.origin[axis]) * invDir[axis];

			// Get node children pointers for ray
			int firstChildIdx = -1;
			int secondChildIdx = -1;
			int belowFirst =
				(ray.origin[axis] < node.splitPos) ||
				(ray.origin[axis] == node.splitPos && ray.direction[axis] <= 0); //???
			if (belowFirst)
			{
				firstChildIdx = currentNodeIdx + 1;
				secondChildIdx = node.aboveChildOffset;
			}
			else
			{
				firstChildIdx = node.aboveChildOffset;
				secondChildIdx = currentNodeIdx + 1;
			}

			// Advance to next child node, possibly enqueue other child
			if (tPlane > tMax || tPlane <= 0)
			{
				currentNodeIdx = firstChildIdx;
			}
			else if (tPlane < tMin)
			{
				currentNodeIdx = secondChildIdx;
			}
			else
			{
				currentNodeIdx = firstChildIdx;

				// Enqueue secondChild in todo list
				nodesToVisit.emplace(secondChildIdx, tPlane, tMax);

				tMax = tPlane;
			}
		}
	}

	return bHit;
}

bool KDTreeAccelerator::IntersectBruteForce(const Ray& ray, float& dist, bool& bBackFace) const
{
	bool hit = false;

	for (auto Primitive : primitives)
	{
		if (Primitive->Intersect(ray, dist, bBackFace))
		{
			ray.maxDist = dist;

			hit = true;
		}
	}

	return hit;
}