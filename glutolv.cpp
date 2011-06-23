#include <GL/glu.h>
#include "OnlineViewer_impl.h"
#include "GLmodel.h"

using namespace OpenHRP;

void glmain(int argc, char *argv[]);

int main(int argc, char *argv[])
{
    CORBA::ORB_var orb = CORBA::ORB::_nil();
  
    try {

	orb = CORBA::ORB_init(argc, argv);
	
	CORBA::Object_var obj;
	
	obj = orb->resolve_initial_references("RootPOA");
	PortableServer::POA_var poa = PortableServer::POA::_narrow(obj);
	if(CORBA::is_nil(poa)){
	    throw std::string("error: failed to narrow root POA.");
	}
	
	PortableServer::POAManager_var poaManager = poa->the_POAManager();
	if(CORBA::is_nil(poaManager)){
	    throw std::string("error: failed to narrow root POA manager.");
	}
	
	OnlineViewer_impl* OnlineViewerImpl = new OnlineViewer_impl(orb, poa);
	poa->activate_object(OnlineViewerImpl);
	OnlineViewer_var OnlineViewer = OnlineViewerImpl->_this();
	OnlineViewerImpl->_remove_ref();

	obj = orb->resolve_initial_references("NameService");
	CosNaming::NamingContext_var namingContext = CosNaming::NamingContext::_narrow(obj);
	if(CORBA::is_nil(namingContext)){
	    throw std::string("error: failed to narrow naming context.");
	}
	
	CosNaming::Name name;
	name.length(1);
	name[0].id = CORBA::string_dup("OnlineViewer");
	name[0].kind = CORBA::string_dup("");
	namingContext->rebind(name, OnlineViewer);

	poaManager->activate();
	
        glmain(argc, argv);
    }
    catch (CORBA::SystemException& ex) {
        std::cerr << ex._rep_id() << std::endl;
    }
    catch (const std::string& error){
        std::cerr << error << std::endl;
    }

    try {
	orb->destroy();
    }
    catch(...){

    }
    
    return 0;
}
