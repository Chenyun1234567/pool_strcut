#include <sys/epoll.h>
#include <functional>
#include <chrono>
#include <set>
#include <memory>
#include <iostream>
#include <unistd.h>

using namespace std;

 typedef struct timernodebase_t{

    uint64_t expire;
    uint64_t id;
}timernodebase;

struct timernode :public timernodebase{
    using Callback=std::function<void (const timernode & node)>;
    Callback func;
    timernode(uint64_t id,uint64_t expire,Callback func):func(func) {
       this->expire=expire;
       this->id=id;
    }

};


bool operator < (const timernodebase&lhd, const timernodebase &rhd) {
    if (lhd.expire < rhd.expire)
        return true;
    else if (lhd.expire > rhd.expire) 
        return false;
    return lhd.id < rhd.id;
}


class CTimer{
   
public:
   static uint64_t GetTick();
   timernodebase AddTimer(uint64_t msec,timernode::Callback func);
   bool DelTimer(timernodebase &node);
   bool CheckTimer();
   uint64_t TimeToSleep();

private:
  static uint64_t get_current_timer_id();
  set<timernode,less<>>timerset;
  static uint64_t current_timer_id;
};

uint64_t CTimer::current_timer_id=0;

uint64_t CTimer::get_current_timer_id(){ 
    return CTimer::current_timer_id++;
}

uint64_t CTimer::GetTick(){

    auto sc = chrono::time_point_cast<chrono::milliseconds>(chrono::steady_clock::now());
    auto temp = chrono::duration_cast<chrono::milliseconds>(sc.time_since_epoch());
    return temp.count();

}


timernodebase CTimer::AddTimer(uint64_t msec,timernode::Callback func){
    uint64_t expire=CTimer::GetTick()+msec;
    auto ele=timerset.emplace(get_current_timer_id(),expire,std::move(func));
    return static_cast<timernodebase>(*ele.first);
}

bool CTimer::DelTimer(timernodebase& node){
    auto iter=timerset.find(node);
    if(iter !=timerset.end()){
        timerset.erase(iter);
        return true;
    }
    return false;
}

bool CTimer::CheckTimer(){
    auto iter = timerset.begin();
    if (iter != timerset.end() && iter->expire <= CTimer::GetTick()) {
            iter->func(*iter);
            timerset.erase(iter);
            return true;
        }
        return false;
}

uint64_t CTimer::TimeToSleep(){
    auto iter = timerset.begin();
    if (iter == timerset.end()) {
            return -1;
        }
    uint64_t diss = iter->expire-GetTick();
    return diss > 0 ? diss : 0;
 }


void Callback(){
    std::cout<<"timer expired!"<<std::endl;
}
// int main(void){

//    unique_ptr<CTimer> timer = make_unique<CTimer>();
   
//    int i=0;
//    timer->AddTimer(1000, [&](const timernode &node) {
//         cout << CTimer::GetTick() << "node id:" << node.id << " revoked times:" << ++i << endl;
//     });
//    timer->AddTimer(5000, [&](const timernode &node) {
//         cout << CTimer::GetTick() << "node id:" << node.id << " revoked times:" << ++i << endl;
//     });

//     timer->AddTimer(3000, [&](const timernode &node) {
//         cout << CTimer::GetTick() << "node id:" << node.id << " revoked times:" << ++i << endl;
//     });

//     auto node = timer->AddTimer(2100, [&](const timernode &node) {
//         cout << CTimer::GetTick() << "node id:" << node.id << " revoked times:" << ++i << endl;
//     });

//    while(!timer->CheckTimer()){
//        usleep(300000);
//        cout << "now time:" << CTimer::GetTick() << endl;
//    }

//     return 0;
// }