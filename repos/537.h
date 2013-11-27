#include <stdint.h>
#include <vector>
#include "ns3/node.h"
#include"node-container.h"
namespace ns3 {

struct nodemap{
  int id;
  int LTT;
  nodemap *next;
};

class NodMapContainer: public NodeContainer{

    public:
        void initmap();
        void addnode(int id,int LTT);
        void updatemap(int id,int LTT);

    private:
        nodemap head;
        int id;
}

NodeMapContainer::initmap(){
    head=new nodemap;
    head->id=id;
    head->LTT=0;
    head->next=NULL;
}

NodeMapContainer::addnode(int id, int LTT){
    nodemap *p=head;
    while(p->next!=NULL)
        p=p->next;
    nodemap temp=new nodemap;
    temp->id=id;
    temp->LTT=LTT;
    temp->nexy=NULL;
    p->next=temp;
}

NodeMapContainer::updatemap(int id,int LTT){
    nodemap *p=head->next;
    while(p!=NULL){
        if(p->id==id)
            p->LTT=LTT;
        p=p->next;
    }
}


}

