

#include "witness.h"
#include "cellapp.h"
#include "entitydef/method.h"
#include "clients_remote_entity_method_ctrl.h"
#include "network/bundle.h"
#include "network/network_stats.h"
#include "helper/eventhistory_stats.h"

#include "client_lib/client_interface.h"
#include "../../server/baseapp/baseapp_interface.h"

namespace KBEngine{

SCRIPT_METHOD_DECLARE_BEGIN(ClientsRemoteEntityMethodCtrl)
SCRIPT_METHOD_DECLARE_END()

SCRIPT_MEMBER_DECLARE_BEGIN(ClientsRemoteEntityMethodCtrl)
SCRIPT_MEMBER_DECLARE_END()

SCRIPT_GETSET_DECLARE_BEGIN(ClientsRemoteEntityMethodCtrl)
SCRIPT_GETSET_DECLARE_END()
SCRIPT_INIT(ClientsRemoteEntityMethodCtrl, tp_call, 0, 0, 0, 0)	

//-------------------------------------------------------------------------------------
ClientsRemoteEntityMethodCtrl::ClientsRemoteEntityMethodCtrl(PropertyDescription* pComponentPropertyDescription, 
													const ScriptDefModule* pScriptModule,
													MethodDescription* methodDescription,
													 ENTITY_ID id, ENTITY_ID ctrl_id):
script::ScriptObject(getScriptType(), false),
pComponentPropertyDescription_(pComponentPropertyDescription),
pScriptModule_(pScriptModule),
methodDescription_(methodDescription),
id_(id),
ctrl_id_(ctrl_id)
{
}

//-------------------------------------------------------------------------------------
ClientsRemoteEntityMethodCtrl::~ClientsRemoteEntityMethodCtrl()
{
}

//-------------------------------------------------------------------------------------
PyObject* ClientsRemoteEntityMethodCtrl::tp_call(PyObject* self, PyObject* args, 
	PyObject* kwds)	
{
	ClientsRemoteEntityMethodCtrl* rmethod = static_cast<ClientsRemoteEntityMethodCtrl*>(self);
	return rmethod->callmethod(args, kwds);	
}		

//-------------------------------------------------------------------------------------
PyObject* ClientsRemoteEntityMethodCtrl::callmethod(PyObject* args, PyObject* kwds)
{
	// 获取entityView范围内其他entity
	// 向这些entity的client推送这个方法的调用
	MethodDescription* methodDescription = getDescription();

	Entity* pEntity = Cellapp::getSingleton().findEntity(id_);
	if(pEntity == NULL || /*pEntity->pWitness() == NULL ||*/
		pEntity->isDestroyed() /*|| pEntity->clientEntityCall() == NULL*/)
	{
		//WARNING_MSG(fmt::format("EntityRemoteMethod::callClientMethod: not found entity({}).\n", 
		//	entityCall->id()));

		S_Return;
	}
	
	// 先发给自己
	if(methodDescription->checkArgs(args))
	{
		MemoryStream* mstream = MemoryStream::createPoolObject();

		// 如果是广播给组件的消息
		if (pComponentPropertyDescription_)
		{
			if (pScriptModule_->usePropertyDescrAlias())
				(*mstream) << pComponentPropertyDescription_->aliasIDAsUint8();
			else
				(*mstream) << pComponentPropertyDescription_->getUType();
		}
		else
		{
			if (pScriptModule_->usePropertyDescrAlias())
				(*mstream) << (uint8)0;
			else
				(*mstream) << (ENTITY_PROPERTY_UID)0;
		}

		methodDescription->addToStream(mstream, args);

		Entity* pViewEntity = Cellapp::getSingleton().findEntity((ctrl_id_));
		if(pViewEntity == NULL || pViewEntity->pWitness() == NULL || pViewEntity->isDestroyed()) {
            ERROR_MSG(fmt::format("ClientsRemoteEntityMethodCtrl::callmethod, pViewEntity is NULL or pWitness is NULL or pViewEntity is destroyed.... ctrl_id: {}, self id: {}\n", ctrl_id_, id_));
            S_Return;
        }
            
		EntityCall* entityCall = pViewEntity->clientEntityCall();
		if(entityCall == NULL) {
            ERROR_MSG(fmt::format("ClientsRemoteEntityMethodCtrl::callmethod, entityCall is NULL ctrl_id: {}, self id: {}\n", ctrl_id_, id_));
            S_Return;
        }

		Network::Channel* pChannel = entityCall->getChannel();
		if(pChannel == NULL) {
            ERROR_MSG(fmt::format("ClientsRemoteEntityMethodCtrl::callmethod, pChannel is NULL ctrl_id: {}, self id: {}\n", ctrl_id_, id_));
            S_Return;
        }

		// 这个可能性是存在的，例如数据来源于createWitnessFromStream()
		// 又如自己的entity还未在目标客户端上创建
		if (!pViewEntity->pWitness()->entityInView(pEntity->id())) {
            ERROR_MSG(fmt::format("ClientsRemoteEntityMethodCtrl::callmethod, entityInView false ctrl_id: {}, self id: {}\n", ctrl_id_, id_));
            S_Return;
        }
			
		Network::Bundle* pSendBundle = pChannel->createSendBundle();
		NETWORK_ENTITY_MESSAGE_FORWARD_CLIENT_BEGIN(pViewEntity->id(), (*pSendBundle));
			
		int ialiasID = -1;
		const Network::MessageHandler& msgHandler = 
		pViewEntity->pWitness()->getViewEntityMessageHandler(ClientInterface::onRemoteMethodCall, 
				ClientInterface::onRemoteMethodCallOptimized, pEntity->id(), ialiasID);

		ENTITY_MESSAGE_FORWARD_CLIENT_BEGIN(pSendBundle, msgHandler, viewEntityMessage);

		if(ialiasID != -1)
		{
			KBE_ASSERT(msgHandler.msgID == ClientInterface::onRemoteMethodCallOptimized.msgID);
			(*pSendBundle)  << (uint8)ialiasID;
		}
		else
		{
			KBE_ASSERT(msgHandler.msgID == ClientInterface::onRemoteMethodCall.msgID);
			(*pSendBundle)  << pEntity->id();
		}

		if(mstream->wpos() > 0)
			(*pSendBundle).append(mstream->data(), (int)mstream->wpos());

		if(Network::g_trace_packet > 0)
		{
			if(Network::g_trace_packet_use_logfile)
				DebugHelper::getSingleton().changeLogger("packetlogs");

			DEBUG_MSG(fmt::format("ClientsRemoteEntityMethodCtrl::callmethod: pushUpdateData: ClientInterface::onRemoteOtherEntityMethodCall({}::{})\n", 
				pViewEntity->scriptName(), methodDescription->getName()));

			switch(Network::g_trace_packet)	
			{
			case 1:
				mstream->hexlike();
				break;
			case 2:
				mstream->textlike();
				break;
			default:
				mstream->print_storage();
				break;
			};

			if(Network::g_trace_packet_use_logfile)
				DebugHelper::getSingleton().changeLogger(COMPONENT_NAME_EX(g_componentType));
		}

		ENTITY_MESSAGE_FORWARD_CLIENT_END(pSendBundle, msgHandler, viewEntityMessage);

		// 记录这个事件产生的数据量大小
		g_publicClientEventHistoryStats.trackEvent(pViewEntity->scriptName(), 
			methodDescription->getName(), 
			pSendBundle->currMsgLength(), 
			"::");

		pViewEntity->pWitness()->sendToClient(ClientInterface::onRemoteMethodCallOptimized, pSendBundle);

		MemoryStream::reclaimPoolObject(mstream);
	}

	S_Return;
}

//-------------------------------------------------------------------------------------

}


