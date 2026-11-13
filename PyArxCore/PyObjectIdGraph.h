#pragma once
#include "stdafx.h"
#include "idgraph.h"

// AcDbObjectIdGraph pointer wrapper (idgraph.h; object-reference dependency
// graph). Ids cross the Python boundary as uint64 old-style ids (0 == null).
// The graph owns its AcDbObjectIdGraphNode children once added.
class PyObjectIdGraph {
public:
	explicit PyObjectIdGraph(AcDbObjectIdGraph* p = nullptr, bool own = false);
	~PyObjectIdGraph();
	PyObjectIdGraph(const PyObjectIdGraph&) = delete;
	PyObjectIdGraph& operator=(const PyObjectIdGraph&) = delete;

	static std::unique_ptr<PyObjectIdGraph> create();

	AcDbObjectIdGraph* impObj() const { return m_ptr; }
	bool isNull() const { return m_ptr == nullptr; }

	int numNodes() const;
	bool isEmpty() const;
	// Add a node for an object id; returns false on failure/null graph.
	bool addNode(uint64_t objectId);
	// Remove the node for an object id; returns false when absent.
	bool delNode(uint64_t objectId);
	// Find the node's object id; returns 0 when absent.
	uint64_t findNode(uint64_t objectId) const;
	// Object id of the node at index; returns 0 on out-of-range/null.
	uint64_t idNode(int index) const;
	AcString toString() const;
private:
	AcDbObjectIdGraph* m_ptr = nullptr;
	bool m_own = false;
};

void bind_PyObjectIdGraph(pym& m);
