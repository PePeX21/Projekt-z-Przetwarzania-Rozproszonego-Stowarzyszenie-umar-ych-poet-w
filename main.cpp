#include <iostream>
#include <cstdlib>
#include <math.h>
#include <mpi.h>
#include <pthread.h>
#include <vector>
#include <string>
#include <unistd.h>
#include <algorithm>
#include <fcntl.h>


#define MESS 1
#define JOIN_REQUEST 5
#define GROUP_CREATED 6
#define JOIN_ACCEPTED 7
#define JOIN_DENIED 8
#define GROUP_CLOSED 9
#define SENDING_ITEMS 10
#define NEW_JOIN 11
#define GROUP_INFO_SEND 12
#define REQUEST 20
#define REPLY 21
#define RELEASE 22
#define DELETE 23
#define AFTER 24

using namespace std;


int gotFromReceived;
int gotFrom;
int pLiderId;
int random_number;
int delay = 1000000;

int process_size, process_rank;
MPI_Status status;

int p, w;
int maxGroup;
int existingGroups;
	
bool notEnded = true;
bool pGotGroup = false;
bool pLooking = true;
bool pGroup = false;
bool pLider = false;
bool pPartyTime = false;
bool pChoosingBringer = false;
bool group_open = false;
bool pBringing = false;
bool bWaitingForLiderReply = false;

bool next_one = false;

vector<int> poets_in_group;

struct items{
	int rank;
	int time_from_none = 0;
	int time_from_food = 0;
	int time_from_alcohol = 0;
	
}self_items;

vector<items> group_items_list;

struct message{
	int lamport;
	int sender;
	int message_type;
	int newJoin;
	items sent_items;
}messageSend, messageReceived, messageReceivedSend, messageSendCrit;

//do lamporta
int lamport_clock = 0;
int lamport_clockMessage = 0;
int lenOfrequestQueue = 0;
int recivedReply;
vector<message> requestQueue;

//do threadow
pthread_t my_thread;
pthread_t my_thread_critical;
pthread_mutex_t sem_communication = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t sem_queue = PTHREAD_MUTEX_INITIALIZER;



bool compareById(const items &a, const items &b){
	return a.rank < b.rank;
}


bool compareByLength(const message &a, const message &b)
{
	if(a.lamport < b.lamport){
		return true;
	}
	else if(a.lamport == b.lamport && a.sender < b.sender){
		return true;
	}
	return false;
}

void update_requestQueue(message date)
{
	pthread_mutex_lock(&sem_queue);
	requestQueue.insert(requestQueue.begin(), date);
	sort(requestQueue.begin(), requestQueue.end(), compareByLength);
	lenOfrequestQueue ++;
	pthread_mutex_unlock(&sem_queue);
}


void *my_thread_critical_func(void *ptr){
	while(true){
		pthread_mutex_lock( &sem_communication );
	
		srand(time(NULL) + process_rank);
		random_number = rand() % 11;
		
		if(((existingGroups*10)/maxGroup) >= random_number || pLider ||pGotGroup || pGroup){ // or pLider || pGotGroup || pGroup
			cout << "proces " << process_rank << " z zegarem: " << lamport_clock << "- rezygnuje z ubiegania sie o lidera" << endl;
			usleep(delay);
			
			pthread_mutex_lock( &sem_queue ); //--
			for (int i = 0; i < lenOfrequestQueue; i++){
    				if( process_rank ==  requestQueue[i].sender){
    					requestQueue.erase(requestQueue.begin() + i);
    					lenOfrequestQueue --;
    					pthread_mutex_unlock( &sem_queue ); //--
    					break;
    				}
    			}
    					
			messageSendCrit.lamport = lamport_clock;
			lamport_clock ++;
			messageSendCrit.sender = process_rank;
			messageSendCrit.message_type = DELETE;		
			
			for( int i = 0; i < p; i++){
				if(i == process_rank){
					continue;
				}
				MPI_Send(&messageSendCrit, sizeof(message), MPI_BYTE, i, DELETE, MPI_COMM_WORLD);
			}
			
			/*
			srand(time(NULL) + process_rank);
			random_number = rand() % 11;	
			usleep(random_number *2000000);*///////////////////////////////////////////////////////////////////////////////////////////////////////
			
			break;
		}
		if(requestQueue[0].sender == process_rank){			
			cout << "proces " << process_rank << " z zegarem: " << lamport_clock << "- uzyskuje dostep do sekcji krytycznej, zostaje liderem i tworzy grupe. ilosc istniejacych kolek " << existingGroups + 1 << endl;
			existingGroups ++;
			pthread_mutex_lock( &sem_queue ); //--
			lenOfrequestQueue --;
			requestQueue.erase(requestQueue.begin());
			pthread_mutex_unlock( &sem_queue ); //--
			
			messageSendCrit.lamport = lamport_clock;
			lamport_clock ++;
			messageSendCrit.sender = process_rank;
			messageSendCrit.message_type = RELEASE;
											
			for( int i = 0; i < p; i++){
				if(i == process_rank){
					continue;
				}
				MPI_Send(&messageSendCrit, sizeof(message), MPI_BYTE, i, RELEASE, MPI_COMM_WORLD);
			}
			
			pLooking = false;
			pGroup = true;
			pLider = true;
			group_open = true;
			poets_in_group.clear();
			poets_in_group.push_back(process_rank);
					
			messageSendCrit.message_type = GROUP_CREATED;
			messageSendCrit.sender = process_rank;
			messageSendCrit.lamport = lamport_clock;
			lamport_clock ++;
					
			for( int i = 0; i < p; i++){
				if(i == process_rank){
					continue;
				}	
					MPI_Send(&messageSendCrit, sizeof(message), MPI_BYTE, i, GROUP_CREATED, MPI_COMM_WORLD);		
			}				
			pBringing = true;
			
			break;
		}
	}
	return 0;
}


void *thread_com(void *ptr){
	
	
	while(notEnded){
		MPI_Recv(&messageReceived, sizeof(message), MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		lamport_clock = max(lamport_clock, messageReceived.lamport) + 1;
		
		//cout << "--"  << pBringing << " " << pLooking << " " << messageReceived.message_type << " " << process_rank << endl;
		
		if(messageReceived.message_type == REQUEST){
    			
    			update_requestQueue(messageReceived);
    		
    			messageReceivedSend.sender = process_rank;
			messageReceivedSend.lamport = lamport_clock;
			lamport_clock ++;
			messageReceivedSend.message_type = REPLY;
			MPI_Send(&messageReceivedSend, sizeof(message), MPI_BYTE, messageReceived.sender, REPLY, MPI_COMM_WORLD);
			//cout << "-" << process_rank << " wyslal reply do " << messageReceived.sender << endl;
		}
		
		
		if(messageReceived.message_type == RELEASE){
    			
			existingGroups ++; 	
    			pthread_mutex_lock( &sem_queue ); //--		
    			for (int i = 0; i < lenOfrequestQueue; i++){
    				if( messageReceived.sender ==  requestQueue[i].sender){
    					requestQueue.erase(requestQueue.begin() + i);
    					lenOfrequestQueue --;
    					
    					// wyswietlanie listy
    					//for ( int i = 0;  i <lenOfrequestQueue; i++){
					//	cout << "--" <<rankProcces << " widzi QB:  " << requestQueue[i].id << " czas " << requestQueue[i].lamport << endl;
					//}
					pthread_mutex_unlock( &sem_queue ); //--	
    					break;
    				}
			}
			pthread_mutex_unlock( &sem_communication );    
		}
		
		if(messageReceived.message_type == REPLY){
			//cout << "-dostal " << process_rank << " wiadomosc reply od " << messageReceived.sender << "  wlasny radosny zegar" << lamport_clock << endl;	
			if (lamport_clockMessage > messageReceived.lamport){
				cout << "COS SIE OSTRO POJEBALO KOLEZANKO" << endl;
			}
			else{
				recivedReply --;
			}
			
			if (recivedReply == 0){
				recivedReply = p-1;
				pthread_create( &my_thread_critical, NULL, my_thread_critical_func , 0);							
			}
		}
		
		if(messageReceived.message_type == DELETE){ 			
			pthread_mutex_lock( &sem_queue ); //--
			for (int i = 0; i < lenOfrequestQueue; i++){
	    			if( messageReceived.sender ==  requestQueue[i].sender){
	    				requestQueue.erase(requestQueue.begin() + i);
	    				lenOfrequestQueue --;					
	    			}
			}
			pthread_mutex_unlock( &sem_queue ); //--
			pthread_mutex_unlock( &sem_communication );	
		}
		
		if(messageReceived.message_type == AFTER){
			existingGroups--;
			pthread_mutex_unlock( &sem_communication );
		}

		//lider dostaje join request i odsyła akceptacje jeśli grupa jest otwarta
		if(pLider && group_open && messageReceived.message_type ==  JOIN_REQUEST){
			cout << "Lider  " << process_rank <<  " z zegarem: " << lamport_clock << " zaakceptowal prosbe o dolaczenie poety " << messageReceived.sender << "." << endl; 
			usleep(delay);
			gotFrom = messageReceived.sender;
			messageReceivedSend.message_type = JOIN_ACCEPTED;
			messageReceivedSend.sender = process_rank;
			messageReceivedSend.lamport = lamport_clock;
			MPI_Send(&messageReceivedSend, sizeof(message), MPI_BYTE, gotFrom, JOIN_ACCEPTED, MPI_COMM_WORLD);
			lamport_clock ++;
			
			messageReceivedSend.newJoin = gotFrom;
			messageReceivedSend.message_type = NEW_JOIN;
			messageReceivedSend.lamport = lamport_clock;
			lamport_clock ++;	
			for(int i = 0; i < poets_in_group.size(); i++){
				MPI_Send(&messageReceivedSend, sizeof(message), MPI_BYTE, poets_in_group.at(i), NEW_JOIN, MPI_COMM_WORLD);		
			}
		
			poets_in_group.push_back(gotFrom);
		}
		if(pLider && !group_open && messageReceived.message_type == JOIN_REQUEST){
			cout << "Lider  " << process_rank <<  " z zegarem: " << lamport_clock << "- odrzucil prosbe o dolaczenie poety: " << messageReceived.sender << endl;
			usleep(delay);
			messageReceivedSend.message_type = JOIN_DENIED;
			messageReceivedSend.sender = process_rank;
			messageReceivedSend.lamport = lamport_clock;
			MPI_Send(&messageReceivedSend, sizeof(message), MPI_BYTE, gotFrom, JOIN_DENIED, MPI_COMM_WORLD);
			lamport_clock ++;
		}
		
		if(!pLider && !pLooking && pGroup && messageReceived.message_type == NEW_JOIN){
			cout << "Poeta  " << process_rank <<  " z zegarem: " << lamport_clock << "- otrzmal informacje o nowym poecie: " << messageReceived.sender << endl;
			usleep(delay);
			poets_in_group.push_back(messageReceived.newJoin);
			messageReceivedSend.message_type = GROUP_INFO_SEND;
			messageReceivedSend.sender = process_rank;
			messageReceivedSend.lamport = lamport_clock;
			MPI_Send(&messageReceivedSend, sizeof(message), MPI_BYTE, messageReceived.newJoin, GROUP_INFO_SEND, MPI_COMM_WORLD);
			lamport_clock ++;	
		}
		if(!pLider && !pLooking && pGroup && messageReceived.message_type == GROUP_INFO_SEND){
			cout << "Poeta  " << process_rank <<  " z zegarem: " << lamport_clock << "- otrzmal informacje o id innego poety: " << messageReceived.sender << endl;
			usleep(delay);
			poets_in_group.push_back(messageReceived.sender);
		}
		
		//poeta dołącza do grupy i zmienia info
		if(!pLider && !pLooking && !pGroup && pGotGroup && messageReceived.message_type == JOIN_ACCEPTED){
			cout << "Poeta  " << process_rank << " z zegarem: " << lamport_clock << "- otrzymal akceptacje do grupy od lidera: " << messageReceived.sender << endl;
			usleep(delay);
			pLooking = false;
			pGroup = true;
			pGotGroup = false;
			bWaitingForLiderReply = false;
			pLiderId = messageReceived.sender;
			poets_in_group.clear();
			poets_in_group.push_back(pLiderId);
			poets_in_group.push_back(process_rank);
		}
		
		if(!pLider && !pLooking && !pGroup && pGotGroup && messageReceived.message_type == JOIN_DENIED){
			pLooking = true;
			bWaitingForLiderReply = false;
		}
		
		//poeta dostaje informacje o utworzonej grupie i zapisuje od kogo otrzymał, żeby wiedzieć, gdzie wysłać z powrotem
		if(!pLider && pLooking && !pGroup && !pGotGroup && messageReceived.message_type == GROUP_CREATED){

			cout << "Poeta  " << process_rank << " z zegarem: " << lamport_clock << "- otrzymal informacje  o nowej grupie od lidera: " << messageReceived.sender << endl;
			usleep(delay);
			pGotGroup = true;
			gotFrom = messageReceived.sender;
		}
		//dostaje info, że ma przynieść papu i odsyła info (trochę nie wiem czy w tym wątku też robić odsyłanie, lekko się gubię
		if(!pBringing && pGroup && !pLooking && messageReceived.message_type == GROUP_CLOSED){
			pBringing = true;
			group_items_list.clear();
			group_items_list.push_back(messageReceived.sent_items);
			group_items_list.push_back(self_items);
			messageReceivedSend.sender = process_rank;
			messageReceivedSend.sent_items = self_items;
			messageReceivedSend.message_type = SENDING_ITEMS;
			messageReceivedSend.lamport = lamport_clock;
			
			cout << "Poeta  " << process_rank << " z zegarem: " << lamport_clock <<"- otrzymal informacje o zamknieciu grupy" << endl;
			usleep(delay);
			
			//cout << poets_in_group.size()<< endl;
			//odsyłamy nasze info do reszty ziomków, żeby wiedzieli jakie czasy na itemach mamy
			for(int i = 0; i < poets_in_group.size(); i++){
				if(poets_in_group.at(i) != process_rank){
					MPI_Send(&messageReceivedSend, sizeof(message), MPI_BYTE, poets_in_group.at(i), SENDING_ITEMS, MPI_COMM_WORLD);
				}
			} 
			lamport_clock ++;
			
		}
		
		//dostaje info od członka grupy z jego listą itemów i dopisuje do listy
		if(pBringing && pGroup && !pLooking && messageReceived.message_type == SENDING_ITEMS && !pChoosingBringer){
			cout << "Poeta  " << process_rank << " z zegarem: " << lamport_clock <<"- otrzymal informacje o itemach od poety:  " << messageReceived.sender << endl;
			usleep(delay);
			group_items_list.push_back(messageReceived.sent_items);
		
			//jeśli otrzymaliśmy całość info od całej grupy to możemy lecieć z wyborem
			if(group_items_list.size() == poets_in_group.size()){
				pChoosingBringer = true;
				pBringing = false;
			}
		}			
	}
	
	return 0;
}



int main(int argc, char *argv[]){
	
	//inicjalizacja MPI
    	int provided;
    	MPI_Init_thread(&argc, &argv,MPI_THREAD_MULTIPLE, &provided);
    	MPI_Comm_size(MPI_COMM_WORLD, &process_size);
    	MPI_Comm_rank(MPI_COMM_WORLD, &process_rank);

	//podzial na poetow i wolontariuszy
	p = atoi(argv[1]);
	w = atoi(argv[2]);
	maxGroup = p/4+1;
	existingGroups = 0;
	recivedReply = p-1;
	
	//inicjalizacja itemow procesu
	self_items.rank = process_rank;
	
	if(process_rank >= p){
		MPI_Finalize();
		exit(0);
	}
	
	
	pthread_create( &my_thread, NULL, thread_com , 0);
	
	srand(time(NULL) + process_rank);
	int counter = 0;
	
	random_number = rand() % 11;
	if(existingGroups == 0){
		lamport_clockMessage = lamport_clock;
		messageSend.lamport = lamport_clockMessage;
		messageSend.sender = process_rank;
		messageSend.message_type = REQUEST;
		update_requestQueue(messageSend);				
		
		cout << "proces " << process_rank << " z zegarem: " << lamport_clock << "- ubiega sie o sekcje krytyczna by zostac liderem" << endl;
		usleep(delay);
		for( int i = 0; i < p; i++){
			if(i == process_rank){
				continue;
			}
			MPI_Send(&messageSend, sizeof(message), MPI_BYTE, i, MESS, MPI_COMM_WORLD);
		}
		lamport_clock ++; 
	}
	else if(((existingGroups*10)/maxGroup) < random_number){
		lamport_clockMessage = lamport_clock;
		messageSend.lamport = lamport_clockMessage;
		messageSend.sender = process_rank;
		messageSend.message_type = REQUEST;
		update_requestQueue(messageSend);		
	
		cout << "proces " << process_rank << " z zegarem: " << lamport_clock << "- ubiega sie o sekcje krytyczna by zostac liderem" << endl;
		usleep(delay);
		for( int i = 0; i < p; i++){
			if(i == process_rank){
				continue;
			}
			MPI_Send(&messageSend, sizeof(message), MPI_BYTE, i, MESS, MPI_COMM_WORLD);	
		}
		lamport_clock ++; 
	}
		
	while(notEnded){
		
		//jeśli nie jest w grupie, szuka grupy i nie jest liderem to
		if(!pGroup && pLooking && !pLider){
			
			//tu by musiała być zmienna, która sprawdza czy otrzymał wiadomość o istniejącej grupie
			if(pGotGroup){
				int random = rand() % 2;
				counter+1;
				if(random == 1 && bWaitingForLiderReply == false){
					pLooking = false;
					bWaitingForLiderReply = true;
					messageSend.message_type = JOIN_REQUEST;
					messageSend.sender = process_rank;
					messageSend.lamport = lamport_clockMessage;
					
					//TRZEBA ZMIENIĆ  DANE W TYM, BO IDK CO WPISAĆ W SUMIE W TEN SEND GDZIENIEGDZIE xd
					cout << "Poeta  " << process_rank <<  " z zegarem: " << lamport_clock << "- odpowiada na propozycje o dolaczenie do grupy lidera: " << gotFrom  << endl;
					usleep(delay);
					
					MPI_Send(&messageSend, sizeof(message), MPI_BYTE, gotFrom, JOIN_REQUEST, MPI_COMM_WORLD);
					lamport_clock ++;
				}
				else if(bWaitingForLiderReply == false){
					//jak nie chce to nie dołączy
					pGotGroup = false;
				}
			}
			else{
				//jeśli nie znalazł grupy, to powinien zostać liderem, ale to jakoś lamportem by trzeba było?				
				
				//poki co lider na ranku robiony!!!!!!!!!!!!
				next_one = false;
				pthread_mutex_lock( &sem_queue ); //--
				for(int i = 0; i < lenOfrequestQueue; i++){
					if(process_rank == requestQueue[i].sender){ /// to trzeba zajebac w semaforach
						next_one = true;
						break;	
					}
				}
				pthread_mutex_unlock( &sem_queue ); //--
				if(next_one){
					continue;
				}
				else{
				
					lamport_clockMessage = lamport_clock;
					lamport_clock ++; // to samo jak po sendach
					messageSend.lamport = lamport_clockMessage;
					messageSend.sender = process_rank;
					messageSend.message_type = REQUEST;
					update_requestQueue(messageSend);
					
					cout << "proces " << process_rank << " z zegarem: " << lamport_clock << "- ubiega sie o sekcje krytyczna by zostac liderem" << endl;	
					usleep(delay);
					for( int i = 0; i < p; i++){
						if(i == process_rank){
							continue;
						}
						MPI_Send(&messageSend, sizeof(message), MPI_BYTE, i, MESS, MPI_COMM_WORLD);
					}
				}
			}
		}
		
		if(pLider){
			//timer na zamykanie grup
			while(group_open){
				messageSend.message_type = GROUP_CREATED;
				messageSend.sender = process_rank;
				messageSend.lamport = lamport_clock;		
				
				for(int k = 0; k<=p; k++){	
					if(process_rank != k){
							
						MPI_Send(&messageSend, sizeof(message), MPI_BYTE, k, GROUP_CREATED, MPI_COMM_WORLD);	
					}
				}
				lamport_clock ++;
				if(poets_in_group.size() > 2){
					usleep(5000000);
					group_open = false;
					pBringing = true;
					
					//odsyłamy wiadomości do wszystkich członków grupy
					messageSend.sender = process_rank;
					messageSend.message_type = GROUP_CLOSED;
					messageSend.sent_items = self_items;
					messageSend.lamport = lamport_clock;
					//messageSend.group = poets_in_group;
					group_items_list.clear();
					group_items_list.push_back(self_items);
					
					cout << "Lider  " << process_rank << " z zegarem: " << lamport_clock <<"- zamyka grupe" << endl;
					usleep(delay);
					
					for(int i = 0; i < poets_in_group.size(); i++){
						if(poets_in_group.at(i) != process_rank){
							int innerRank = poets_in_group.at(i);
							MPI_Send(&messageSend, sizeof(message), MPI_BYTE, innerRank, GROUP_CLOSED, MPI_COMM_WORLD);
						}
					}
					lamport_clock ++;	
				}
			}
			
		}
		
		if(pChoosingBringer){
			
			int minfood = -1;
			int minalcohol = -1;
			int minnonealc = -1;
			int minnonefood = -1;
			int rankfood = -1;
			int rankalcohol = -1;
			
			//TODO TRZEBA BY BYŁO POSORTOWAĆ TĄ LISTĘ PO RANKACH, BO TO SIĘ WYKRZACZY JAK W INNEJ KOLEJNOŚCI BĘDĄ PRZETWARZAĆ(CHYBA)
			sort(group_items_list.begin(), group_items_list.end(), compareById);
				
			for(int j = 0; j < group_items_list.size(); j++){
				
				if(group_items_list[j].time_from_food > minfood){
					minfood = group_items_list[j].time_from_food;
					rankfood = group_items_list[j].rank;
					minnonefood = group_items_list[j].time_from_none;
					if(rankfood == process_rank){
						cout << "Poeta " << process_rank << " przynosi zagryche." << endl;
						usleep(delay);
					}
				}
				
				//sprawdzamy dla wersji, gdzie mają takie same wartości i jeden więcej razy nic nie przynosił 
				if(group_items_list[j].time_from_food == minfood){
					
					if(group_items_list[j].time_from_none > minnonefood){
						minfood = group_items_list[j].time_from_food;
						rankfood = group_items_list[j].rank;
						minnonefood = group_items_list[j].time_from_none;
						if(rankfood == process_rank){
							cout << "Poeta "<< process_rank << " przynosi zagryche." << endl;
							usleep(delay);
						}	
					}
				}
				
				//sprawdzamy sobie czy przypadkiem nam ten rank się wyżej nie użył - wtedy by nie mógł przynieść
				if(group_items_list[j].time_from_alcohol > minalcohol && rankfood != group_items_list[j].rank){
					minalcohol = group_items_list[j].time_from_alcohol;
					rankalcohol = group_items_list[j].rank;
					minnonealc = group_items_list[j].time_from_none;
					if(rankalcohol == process_rank){
						cout << "Poeta "<< process_rank << " przynosi alkohol." << endl;
						usleep(delay);
					}
				}
				
				if(group_items_list[j].time_from_alcohol == minalcohol && rankfood != group_items_list[j].rank){
					if(group_items_list[j].time_from_none > minnonefood){
						minalcohol = group_items_list[j].time_from_alcohol;
						rankalcohol = group_items_list[j].rank;
						minnonealc = group_items_list[j].time_from_none;
						if(rankalcohol == process_rank){
							cout << "Poeta "<< process_rank << " przynosi alkohol." << endl;
							usleep(delay);
						}
					}
				}
			}
			
			if(rankfood == process_rank){
				self_items.time_from_food = 0;
			}
			else if(rankalcohol == process_rank){
				self_items.time_from_alcohol = 0;
			}
			else{
				self_items.time_from_alcohol = self_items.time_from_alcohol + 1;
				self_items.time_from_food = self_items.time_from_food + 1;
				self_items.time_from_none = self_items.time_from_none + 1;
			}
			
			//zaczynamy party
			pChoosingBringer = false;
			pPartyTime = true;
		}
		
		if(pPartyTime){
		
			cout << "Poeta  " << process_rank << " z zegarem: " << lamport_clock <<"- zakonczyl libacje" << endl;
			usleep(delay*5);
			
			
			if(pLider){
				existingGroups --;
				messageSendCrit.lamport = lamport_clock;
				lamport_clock ++;
				messageSend.sender = process_rank;
				messageSend.message_type = AFTER;
				for( int i = 0; i < p; i++){
					if(i == process_rank){
						continue;
					}
				MPI_Send(&messageSend, sizeof(message), MPI_BYTE, i, AFTER, MPI_COMM_WORLD);
			}
			}
			pGotGroup = false;
			pLooking = true;
			pGroup = false;
			pLider = false;
			pPartyTime = false;
			pChoosingBringer = false;
			group_open = false;
			pBringing = false;
						
			
		}		
	}	
	
	
	pthread_join(my_thread,NULL);
	pthread_mutex_destroy(&sem_communication);
	MPI_Finalize();
	return 0;
}

