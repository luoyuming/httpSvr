#pragma once
#include "tinyxml/tinyxml.h"
#include "tinyxml/tinystr.h"

#include "singleton.h"
#include "common.h"
#include "util.h"
#include "log.h"
#include "msgDef.h"



static const string  XML_NODE_CONFIG = "config";
static const string  XML_NODE_NAME = "item";
static const string  XML_FILE_SYSTEM = "system.xml";


class CHandleXml {
	
public:	
    bool init();
    XML_SYSTEM_CONFIG getSystemXml();
	string getPath();
	string getPahtEx();
    int getThreadEvnetNum();
private:
    bool readDefaultXml();
	bool readXmlFile(string & strFilename);
	bool GetNodePointerByName(TiXmlElement* pRootEle,
		const char* strNodeName, TiXmlElement* &destNode);
	bool serialSystemXml(XML_SYSTEM_CONFIG & data, vector<string> & vecInfo);
	
private: 
	std::mutex                      m_lock;
    XML_SYSTEM_CONFIG				m_xml_system;
	string                          m_strPath;
};

