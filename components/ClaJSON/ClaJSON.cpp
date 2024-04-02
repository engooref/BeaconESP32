#include <ClaJSON.h>
#include <string>

using namespace std;

int main() {

	ClaJSON testJSON, *testElem;
	vector<ClaJSON*> testTab;
	int test = 15;
	bool testBool = true;
	string tests = "test";

	testElem = new ClaJSON;
	testElem->Add("testInt", &test, e_int);
	testElem->Add("testBool", &testBool, e_bool);
	testElem->Add("testStr", (void*)&tests, e_str);

	testJSON.Add("testElem", (void*)testElem, e_objectOrNull);
	
	testTab.push_back(testElem);
	testTab.push_back(testElem);
	testJSON.Add("tabElem", (void*)&testTab, e_tab);
	
	string testSerialize = ClaJSON::Serialize(testJSON);
	ClaJSON testDes(testSerialize);
	delete testElem;
	
	return 0; 
}


ClaJSON::NodeData::NodeData(NodeData* pPrev, NodeData* pNext, string name, e_typeData type) :
	m_pPrev(pPrev),
	m_pNext(pNext),
	m_nameProperties(name),
	m_typeData(type)
{
	if (m_pPrev) m_pPrev->m_pNext = this;
	if (m_pNext) m_pNext->m_pPrev = this;
}

ClaJSON::NodeData::~NodeData()
{
	if (m_pPrev) m_pPrev->m_pNext = m_pNext;
	if (m_pNext) m_pNext->m_pPrev = m_pPrev;
}

ClaJSON::NodeInt::NodeInt(NodeData* pPrev, NodeData* pNext, string name, int properties) :
	NodeData(pPrev, pNext, name, e_int),
	m_properties(properties)
{
}

ClaJSON::NodeInt::~NodeInt()
{

}

ClaJSON::NodeStr::NodeStr(NodeData* pPrev, NodeData* pNext, string name, string properties) :
	NodeData(pPrev, pNext, name, e_str),
	m_properties(properties)
{
}

ClaJSON::NodeStr::~NodeStr()
{
}

ClaJSON::NodeBool::NodeBool(NodeData* pPrev, NodeData* pNext, string name, bool properties) :
	NodeData(pPrev, pNext, name, e_bool),
	m_properties(properties)
{
}

ClaJSON::NodeBool::~NodeBool()
{
}

ClaJSON::NodeObjectOrNull::NodeObjectOrNull(NodeData* pPrev, NodeData* pNext, string name, ClaJSON* properties) :
	NodeData(pPrev, pNext, name, e_objectOrNull),
	m_properties(properties)
{
}

ClaJSON::NodeObjectOrNull::~NodeObjectOrNull()
{
	if (m_properties != nullptr) { delete m_properties; m_properties = nullptr; }
}

ClaJSON::NodeTab::NodeTab(NodeData* pPrev, NodeData* pNext, string name) :
	NodeData(pPrev, pNext, name, e_tab)
{
}

ClaJSON::NodeTab::~NodeTab()
{
	for (auto intvec = m_tabProperties.begin(); intvec < m_tabProperties.end(); intvec++)
		delete (*intvec);
}

void ClaJSON::NodeTab::AddElem(ClaJSON* elem)
{
	if (elem != nullptr)
		m_tabProperties.push_back(new ClaJSON(elem));
}

ClaJSON::ClaJSON() :
	m_pHead(nullptr),
	m_pTail(nullptr),
	m_nCard(0)
{

}

ClaJSON::ClaJSON(ClaJSON* copy) :
	ClaJSON()
{
	NodeData* nodeData = copy->m_pHead;
	void* value = nullptr;

	while (nodeData) {
		switch (nodeData->m_typeData) {
		case e_int:
			value = (void*)&((NodeInt*)nodeData)->m_properties;
			break;
		case e_bool:
			value = (void*)&((NodeBool*)nodeData)->m_properties;

			break;
		case e_str:
			value = (void*)&((NodeStr*)nodeData)->m_properties;

			break;
		case e_objectOrNull:
			value = (void*)((NodeObjectOrNull*)nodeData)->m_properties;

			break;
		case e_tab:
			value = (void*)&((NodeTab*)nodeData)->m_tabProperties;

			break;
		}
		Add(nodeData->m_nameProperties, value, nodeData->m_typeData);
		nodeData = nodeData->m_pNext;
	}
}

ClaJSON::ClaJSON(std::string serializeData) :
	ClaJSON()
{
	if ((serializeData[0] != '{') || (serializeData[serializeData.length() - 1] != '}'))
		return;
	size_t startName = 0, lastName = 0, i = 0, end = 0;
	string nameParam, param;

	while (end != serializeData.length()) {

		startName = serializeData.find("\"", end);
		if (startName == serializeData.npos) break;
		startName++;
		lastName = serializeData.find("\"", startName);

		nameParam = serializeData.substr(startName, (lastName - startName));

		for (i = 1; serializeData[lastName + i] == ' ' || serializeData[lastName + i] == ':'; i++);

		if ((serializeData[lastName + i] != '[') && (serializeData[lastName + i] != '{')) {
			end = serializeData.find(",", (lastName + i));
			if (end == serializeData.npos) end = serializeData.length();
			param = serializeData.substr((lastName + i), end - (lastName + i));

			if (param[0] == 'N') {
				Add(nameParam, nullptr, e_objectOrNull);
			}
			else if ((param[0] == 't') || (param[0] == 'f')) {
				bool valBool = (param[0] == 't');

				Add(nameParam, (void*)&valBool, e_bool);
			}
			else if (param[0] == '\"') {
				param = param.substr(1, param.rfind("\"") - 1);
				Add(nameParam, (void*)&param, e_str);
			} 
			else {
				int valInt = stoi(param);
				Add(nameParam, (void*)&valInt, e_int);
			}
		}
		else if (serializeData[lastName + i] == '{') {
			size_t x = (lastName + i);
			param = serializeData[x];
			x += 1;
			for (size_t y = 1 ; (y != 0) && (x < serializeData.length()); x++) {
				if (serializeData[x] == '{') y++;
				else if (serializeData[x] == '}') y--;

				param += serializeData[x];
			}

			ClaJSON NewObject(param);

			Add(nameParam, (void*)&NewObject, e_objectOrNull);
			end = x;
		}
		else if (serializeData[lastName + i] == '[') {
			size_t x = lastName + i;
			vector<ClaJSON*> vec;

			param = serializeData[x];
			x += 1;
			for (size_t y = 1; (y != 0) && (x < serializeData.length()); x++) {
				if (serializeData[x] == '[') y++;
				else if (serializeData[x] == ']') y--;

				param += serializeData[x];
			}

			end = x;
			x = 0;
			string paramObject;
			while (x < param.length()) {
				x = param.find("{", x);
				if (x == param.npos) break;
				paramObject = param[x];
				x++;
				for (size_t y = 1; (y != 0) && (x < param.length()); x++) {
					if (param[x] == '{') y++;
					else if (param[x] == '}') y--;

					paramObject += param[x];
				}
				vec.push_back(new ClaJSON(paramObject));
			}

			Add(nameParam, (void*)&vec, e_tab);
		}
	}
}

ClaJSON::~ClaJSON()
{
	while (m_pHead) {
		m_pTail = m_pHead;
		m_pHead = m_pHead->m_pNext;
		delete m_pTail;
		m_nCard--;
	}

}

bool ClaJSON::Add(std::string name, void* value, e_typeData type)
{
	if (IsExist(name)) { return false; }
	switch (type) {
	case e_int:
		return AddInt(name, *(int*)value);
	case e_bool:
		return AddBool(name, *(bool*)value);
	case e_str:
		return AddStr(name, *(string*)value);
	case e_objectOrNull:
		return AddObject(name, (ClaJSON*)value);
	case e_tab:
		return AddTab(name, (vector<ClaJSON*>*) value);
	default:
		return false;
	}	
}

bool ClaJSON::AddInt(string name, int value)
{
	NodeInt* pNewNode = new NodeInt(m_pTail, nullptr, name, value);
	return InsertLast(pNewNode);
}

bool ClaJSON::AddStr(string name, string value)
{
	NodeStr* pNewNode = new NodeStr(m_pTail, nullptr, name, value);
	return InsertLast(pNewNode);
}

bool ClaJSON::AddBool(string name, bool value)
{
	NodeBool* pNewNode = new NodeBool(m_pTail, nullptr, name, value);
	return InsertLast(pNewNode);
}

bool ClaJSON::AddObject(string name, ClaJSON* value)
{
	NodeObjectOrNull* pNewNode = new NodeObjectOrNull(m_pTail, nullptr, name,  (value != nullptr) ? new ClaJSON(value) : nullptr );
	return InsertLast(pNewNode);

}

bool ClaJSON::AddTab(std::string name, vector<ClaJSON*>* value)
{
	NodeTab* pNewNode = new NodeTab(m_pTail, nullptr, name);
	if (pNewNode) {
		for (auto intvec = value->begin(); intvec < value->end(); intvec++)
			pNewNode->AddElem(*intvec);
	}
	return InsertLast(pNewNode);
}

int ClaJSON::GetInt(string name)
{
	NodeInt* m_pItem = (NodeInt*)GetNode(name);
	if (m_pItem) return m_pItem->m_properties;
	else return 0;
}

string ClaJSON::GetStr(string name)
{
	NodeStr* m_pItem = (NodeStr*)GetNode(name);

	if (m_pItem) return m_pItem->m_properties;
	else return string();
}

bool ClaJSON::GetBool(string name)
{
	NodeBool* m_pItem = (NodeBool*)GetNode(name);

	if (m_pItem) return m_pItem->m_properties;
	else return false;
}

ClaJSON* ClaJSON::GetObjectOrNull(string name)
{
	NodeObjectOrNull* m_pItem = (NodeObjectOrNull*)GetNode(name);

	if (m_pItem) return m_pItem->m_properties;
	else return nullptr;
}

vector<ClaJSON*> ClaJSON::GetTab(string name)
{
	NodeTab* m_pItem = (NodeTab*)GetNode(name);

	if (m_pItem) return m_pItem->m_tabProperties;
	else return vector<ClaJSON*>();
}

bool ClaJSON::IsExist(std::string name)
{
	return (GetNode(name) != nullptr);
}

void ClaJSON::RemoveItem(string name)
{
	NodeData* m_pItem = GetNode(name);
	if (m_pItem) {
		delete m_pItem;
	}
}

string ClaJSON::Serialize(ClaJSON& pJSON)
{

	string result;
	NodeData* pNode = pJSON.m_pHead;

	while (pNode) {
		result += " \"" + pNode->m_nameProperties + "\" : ";
	
		switch (pNode->m_typeData) {
		case e_int:
			result += to_string(((NodeInt*)pNode)->m_properties);
			break;
		case e_bool:
			result += (((NodeBool*)pNode)->m_properties) ? "true" : "false";
			break;
		case e_str:
			result += '\"' + ((NodeStr*)pNode)->m_properties + '\"';
			break;
		case e_objectOrNull:
			if (((NodeObjectOrNull*)pNode)->m_properties != nullptr)
				result += Serialize(*((NodeObjectOrNull*)pNode)->m_properties);
			else
				result += "Null";
			break;
		case e_tab:
			result += " [ ";
			for (auto intvec = ((NodeTab*)pNode)->m_tabProperties.begin(); intvec < ((NodeTab*)pNode)->m_tabProperties.end(); intvec++)
			{
				result +=  " " + Serialize(**intvec) + ",";
			}
			result.pop_back();
			result += "]";
			break;
		}
		result += ",";
		pNode = pNode->m_pNext;
	}
	if(result != string()) result.pop_back();
	result = "{ " + result + " }";
	return result;
}


bool ClaJSON::InsertLast(NodeData* pNode)
{
	if (pNode) {
		if (m_pHead == nullptr) m_pHead = pNode;
		m_pTail = pNode;
		m_nCard++;
		return true;
	}
	else return false;
}

ClaJSON::NodeData* ClaJSON::GetNode(string name)
{
	NodeData* m_pItemAct = m_pHead;

	while ((m_pItemAct != nullptr) && (m_pItemAct->m_nameProperties != name))
		m_pItemAct = m_pItemAct->m_pNext;

	return m_pItemAct;
}

