#pragma once

#include <iostream>
#include <vector>

enum e_typeData {
	e_int = 0,
	e_bool,
	e_str,
	e_objectOrNull,
	e_tab
};

class ClaJSON {
private:


	class NodeData {
	public:
		NodeData* m_pPrev;
		NodeData* m_pNext;
		std::string m_nameProperties;
		e_typeData m_typeData;

	public:
		NodeData(NodeData* pPrev, NodeData* pNext, std::string name, e_typeData type);
		virtual ~NodeData() = 0;
	};

	class NodeInt : public NodeData {
	public:
		int m_properties;
	public:
		NodeInt(NodeData* pPrev, NodeData* pNext, std::string name, int properties);
		virtual ~NodeInt();

	};

	class NodeStr : public NodeData {
	public:
		std::string m_properties;
	public:
		NodeStr(NodeData* pPrev, NodeData* pNext, std::string name, std::string properties);
		virtual ~NodeStr();

	};

	class NodeBool : public NodeData {
	public:
		bool m_properties;
	public:
		NodeBool(NodeData* pPrev, NodeData* pNext, std::string name, bool properties);
		virtual ~NodeBool();

	};

	class NodeObjectOrNull : public NodeData {
	public:
		ClaJSON* m_properties;
	public:
		NodeObjectOrNull(NodeData* pPrev, NodeData* pNext, std::string name, ClaJSON* properties);
		virtual ~NodeObjectOrNull();
	};

	class NodeTab : public NodeData {
	public:
		std::vector<ClaJSON*>  m_tabProperties;
	public:
		NodeTab(NodeData* pPrev, NodeData* pNext, std::string name);
		virtual ~NodeTab();
		void AddElem(ClaJSON* elem);
	};

	NodeData* m_pHead;
	NodeData* m_pTail;
	int m_nCard;

public:
	ClaJSON();
	ClaJSON(ClaJSON* copy);
	ClaJSON(std::string serializeData);
	~ClaJSON();

	
private:
	bool AddInt(std::string name, int value);
	bool AddStr(std::string name, std::string value);
	bool AddBool(std::string name, bool value);
	bool AddObject(std::string name, ClaJSON* value);
	bool AddTab(std::string name, std::vector<ClaJSON*>* value);

public:
	bool Add(std::string name, void* value, e_typeData type);

	int GetInt(std::string name);
	std::string GetStr(std::string name);
	bool GetBool(std::string name);
	ClaJSON* GetObjectOrNull(std::string name);
	std::vector<ClaJSON*> GetTab(std::string name);

	bool IsExist(std::string name);
	void RemoveItem(std::string name);
	
	
private:
	bool InsertLast(NodeData* pNode);
	NodeData* GetNode(std::string name);

public:
	static std::string Serialize(ClaJSON& pCJSON);

};
