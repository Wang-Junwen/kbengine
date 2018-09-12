// Copyright 2008-2018 Yolo Technologies, Inc. All Rights Reserved. https://www.comblockengine.com


#ifndef KBE_CTRL_CLIENTS_H
#define KBE_CTRL_CLIENTS_H

#include "common/common.h"
#include "pyscript/scriptobject.h"
#include "entitydef/common.h"
#include "network/address.h"

//#define NDEBUG
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <iostream>	
#include <map>	
#include <vector>	
// windows include	
#if KBE_PLATFORM == PLATFORM_WIN32
#include <time.h> 
#else
// linux include
#include <errno.h>
#endif
	
namespace KBEngine{

namespace Network
{
class Channel;
class Bundle;
}

class CtrlClient;
class ScriptDefModule;
class PropertyDescription;

class CtrlClientComponent : public script::ScriptObject
{
	/** 子类化 将一些py操作填充进派生类 */
	INSTANCE_SCRIPT_HREADER(CtrlClientComponent, ScriptObject)
public:
	CtrlClientComponent(PropertyDescription* pComponentPropertyDescription, CtrlClient* pCtrlClient);

	~CtrlClientComponent();

	/**
	脚本请求获取属性或者方法
	*/
	PyObject* onScriptGetAttribute(PyObject* attr);

	/**
	获得对象的描述
	*/
	PyObject* tp_repr();
	PyObject* tp_str();

	void c_str(char* s, size_t size);

	ScriptDefModule* pComponentScriptDefModule();

protected:
	CtrlClient* pCtrlClient_;
	PropertyDescription* pComponentPropertyDescription_;
};

class CtrlClient : public script::ScriptObject
{
	/** 子类化 将一些py操作填充进派生类 */
	INSTANCE_SCRIPT_HREADER(CtrlClient, ScriptObject)
public:
	CtrlClient(const ScriptDefModule* pScriptModule, 
		ENTITY_ID eid, ENTITY_ID ctrl_eid);
	
	~CtrlClient();
	
	/** 
		脚本请求获取属性或者方法 
	*/
	PyObject* onScriptGetAttribute(PyObject* attr);						
			
	/** 
		获得对象的描述 
	*/
	PyObject* tp_repr();
	PyObject* tp_str();

    void setControllerId(ENTITY_ID ctrl_eid);
	
	void c_str(char* s, size_t size);
	
	/** 
		获取entityID 
	*/
	ENTITY_ID id() const{ return id_; }
	ENTITY_ID ctrl_id() const{ return ctrl_id_;}
	void setID(int id){ id_ = id; }
	DECLARE_PY_GET_MOTHOD(pyGetID);

	void setScriptModule(const ScriptDefModule*	pScriptModule){ 
		pScriptModule_ = pScriptModule; 
	}

protected:
	const ScriptDefModule*					pScriptModule_;			// 该entity所使用的脚本模块对象

	ENTITY_ID								id_;					// entityID

    ENTITY_ID                               ctrl_id_;               // 控制者的entityID

};

}

#endif // KBE_CTRL_CLIENTS_H
