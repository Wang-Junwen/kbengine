// Copyright 2008-2018 Yolo Technologies, Inc. All Rights Reserved. https://www.comblockengine.com


#include "ctrl_client.h"
#include "entity.h"
#include "cellapp.h"
#include "pyscript/pickler.h"
#include "helper/debug_helper.h"
#include "network/packet.h"
#include "network/bundle.h"
#include "network/network_interface.h"
#include "server/components.h"
#include "client_lib/client_interface.h"
#include "entitydef/method.h"
#include "entitydef/property.h"
#include "entitydef/scriptdef_module.h"
#include "clients_remote_entity_method_ctrl.h"

#include "../../server/baseapp/baseapp_interface.h"
#include "../../server/cellapp/cellapp_interface.h"

namespace KBEngine{

SCRIPT_METHOD_DECLARE_BEGIN(CtrlClientComponent)
SCRIPT_METHOD_DECLARE_END()

SCRIPT_MEMBER_DECLARE_BEGIN(CtrlClientComponent)
SCRIPT_MEMBER_DECLARE_END()

SCRIPT_GETSET_DECLARE_BEGIN(CtrlClientComponent)
SCRIPT_GETSET_DECLARE_END()
SCRIPT_INIT(CtrlClientComponent, 0, 0, 0, 0, 0)

//-------------------------------------------------------------------------------------
CtrlClientComponent::CtrlClientComponent(PropertyDescription* pComponentPropertyDescription, CtrlClient* pCtrlClient):
	ScriptObject(getScriptType(), false),
	pCtrlClient_(pCtrlClient),
	pComponentPropertyDescription_(pComponentPropertyDescription)
{
	Py_INCREF(pCtrlClient_);
}

//-------------------------------------------------------------------------------------
CtrlClientComponent::~CtrlClientComponent()
{
	Py_DECREF(pCtrlClient_);
}

//-------------------------------------------------------------------------------------
ScriptDefModule* CtrlClientComponent::pComponentScriptDefModule()
{
	EntityComponentType* pEntityComponentType = static_cast<EntityComponentType*>(pComponentPropertyDescription_->getDataType());
	return pEntityComponentType->pScriptDefModule();
}

//-------------------------------------------------------------------------------------
PyObject* CtrlClientComponent::onScriptGetAttribute(PyObject* attr)
{
	ENTITY_ID entityID = pCtrlClient_->id();

	Entity* pEntity = Cellapp::getSingleton().findEntity(entityID);
	if (pEntity == NULL)
	{
		PyErr_Format(PyExc_AssertionError, "CtrlClientComponent::onScriptGetAttribute: not found entity(%d).",
			entityID);
		PyErr_PrintEx(0);
		return 0;
	}

	if (!pEntity->isReal())
	{
		PyErr_Format(PyExc_AssertionError, "CtrlClientComponent::onScriptGetAttribute: %s not is real entity(%d).",
			pEntity->scriptName(), pEntity->id());
		PyErr_PrintEx(0);
		return 0;
	}

	wchar_t* PyUnicode_AsWideCharStringRet0 = PyUnicode_AsWideCharString(attr, NULL);
	char* ccattr = strutil::wchar2char(PyUnicode_AsWideCharStringRet0);
	PyMem_Free(PyUnicode_AsWideCharStringRet0);

	ScriptDefModule* pScriptDefModule = pComponentScriptDefModule();
	MethodDescription* pMethodDescription = pScriptDefModule->findClientMethodDescription(ccattr);

	free(ccattr);

	if (pMethodDescription != NULL)
	{
        return NULL;
		//return new ClientsRemoteEntityMethod(pComponentPropertyDescription_, pScriptDefModule, pMethodDescription, false, entityID);
	}
	
	return ScriptObject::onScriptGetAttribute(attr);
}

//-------------------------------------------------------------------------------------
PyObject* CtrlClientComponent::tp_repr()
{
	char s[1024];
	c_str(s, 1024);
	return PyUnicode_FromString(s);
}

//-------------------------------------------------------------------------------------
void CtrlClientComponent::c_str(char* s, size_t size)
{
	kbe_snprintf(s, size, "component_clients id:%d.", pCtrlClient_->id());
}

//-------------------------------------------------------------------------------------
PyObject* CtrlClientComponent::tp_str()
{
	return tp_repr();
}

//-------------------------------------------------------------------------------------
SCRIPT_METHOD_DECLARE_BEGIN(CtrlClient)
SCRIPT_METHOD_DECLARE_END()

SCRIPT_MEMBER_DECLARE_BEGIN(CtrlClient)
SCRIPT_MEMBER_DECLARE_END()

SCRIPT_GETSET_DECLARE_BEGIN(CtrlClient)
SCRIPT_GET_DECLARE("id",							pyGetID,				0,					0)
SCRIPT_GETSET_DECLARE_END()
SCRIPT_INIT(CtrlClient, 0, 0, 0, 0, 0)

//-------------------------------------------------------------------------------------
CtrlClient::CtrlClient(const ScriptDefModule* pScriptModule, 
						ENTITY_ID eid, ENTITY_ID ctrl_eid):
ScriptObject(getScriptType(), false),
pScriptModule_(pScriptModule),
id_(eid),
ctrl_id_(ctrl_eid)
{
}

//-------------------------------------------------------------------------------------
CtrlClient::~CtrlClient()
{
}

//-------------------------------------------------------------------------------------
PyObject* CtrlClient::pyGetID()
{ 
	return PyLong_FromLong(id()); 
}

//-------------------------------------------------------------------------------------
PyObject* CtrlClient::onScriptGetAttribute(PyObject* attr)
{
	Entity* pEntity = Cellapp::getSingleton().findEntity(id_);
	if(pEntity == NULL)
	{
		PyErr_Format(PyExc_AssertionError, "CtrlClient::onScriptGetAttribute: not found entity(%d).", 
			id());
		PyErr_PrintEx(0);
		return 0;
	}

	if(!pEntity->isReal())
	{
		PyErr_Format(PyExc_AssertionError, "CtrlClient::onScriptGetAttribute: %s not is real entity(%d).", 
			pEntity->scriptName(), pEntity->id());
		PyErr_PrintEx(0);
		return 0;
	}
	
	wchar_t* PyUnicode_AsWideCharStringRet0 = PyUnicode_AsWideCharString(attr, NULL);
	char* ccattr = strutil::wchar2char(PyUnicode_AsWideCharStringRet0);
	PyMem_Free(PyUnicode_AsWideCharStringRet0);

	MethodDescription* pMethodDescription = const_cast<ScriptDefModule*>(pScriptModule_)->findClientMethodDescription(ccattr);
	
	if(pMethodDescription != NULL)
	{
		free(ccattr);
		return new ClientsRemoteEntityMethodCtrl(NULL, pScriptModule_, pMethodDescription, id_, ctrl_id_);
	}
	else
	{
		// 是否是组件方法调用
		PropertyDescription* pComponentPropertyDescription = const_cast<ScriptDefModule*>(pScriptModule_)->findComponentPropertyDescription(ccattr);
		if (pComponentPropertyDescription)
		{
			free(ccattr);
			return new CtrlClientComponent(pComponentPropertyDescription, this);
		}
	}

	free(ccattr);
	return ScriptObject::onScriptGetAttribute(attr);
}

//-------------------------------------------------------------------------------------
PyObject* CtrlClient::tp_repr()
{
	char s[1024];
	c_str(s, 1024);
	return PyUnicode_FromString(s);
}

//-------------------------------------------------------------------------------------
void CtrlClient::c_str(char* s, size_t size)
{
	kbe_snprintf(s, size, "clients id:%d.", id_);
}

void CtrlClient::setControllerId(ENTITY_ID ctrl_eid)
{
    ctrl_id_ = ctrl_eid;
}

//-------------------------------------------------------------------------------------
PyObject* CtrlClient::tp_str()
{
	return tp_repr();
}

//-------------------------------------------------------------------------------------

}

