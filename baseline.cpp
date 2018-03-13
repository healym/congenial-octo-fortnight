#include <cstdlib>
#include <iostream>
#include <fstream>
#include <cerrno>
#include <unistd.h>
#include "mpi.h"
#include "pomerize.h"

//run compiled code (for 5 Phils) with mpirun -n 5 program

using namespace std;

//this is how many poems you want each Phil to construct & save
const int MAXMESSAGES = 10; 

//if you change this base, update the Makefile "clean" accordingly to remove old poems
const string fileBase = "outFile"; 

int main ( int argc, char *argv[] ) 
{
  int id; //my MPI ID
  int p;  //total MPI processes
  MPI::Status status;
  MPI_Status statusLeft, statusRight;
  MPI_Request requestLeft, requestRight;
  int tag = 1;
  int give_file = 3;
  int access_file_1 = 4
  int access_file_2 = 5
  int file_request_1 = 6
  int file_request_2 = 7;

  //  Initialize MPI.
  MPI::Init ( argc, argv );

  //  Get the number of total processes.
  p = MPI::COMM_WORLD.Get_size ( );

  //  Determine the rank of this process.
  id = MPI::COMM_WORLD.Get_rank ( );
  
  //Safety check - need at least 2 philosophers to make sense
  if (p < 2) {
	    MPI::Finalize ( );
	    std::cerr << "= [ 2 OR MORE PHILOSOPHERS REQUIRED ] =" << std::endl;
	    return 1; //non-normal exit
  }

  srand(id + time(NULL)); //ensure different seeds...

  int numWritten = 0;
  
  //setup message storage locations
  bool recv_left = false;
  bool recv_right = false;
  int msgIn, msgOut;
  int leftNeighbor = (id - 1 + p) % p;
  int rightNeighbor = (id + 1) % p;

  pomerize P;

  string lFile = fileBase + to_string(leftNeighbor);
  string rFile = fileBase + to_string(rightNeighbor);
  ofstream foutLeft(lFile.c_str(), ios::out | ios::app );
  ofstream foutRight(rFile.c_str(), ios::out | ios::app );

  while (numWritten < MAXMESSAGES) {
    //send 1 test message to each neighbor
    	msgOut = rand() % p; //pick a number/message
	MPI::COMM_WORLD.Send ( &msgOut, 1, MPI::INT, leftNeighbor, tag ); 
    	msgOut = rand() % p; //pick a new number/message
	MPI::COMM_WORLD.Send ( &msgOut, 1, MPI::INT, rightNeighbor, tag ); 
        
    //receive 1 test message from each neighbor
	MPI::COMM_WORLD.Recv ( &msgIn, 1, MPI::INT, MPI::ANY_SOURCE, tag, status );
//	std::cout << "Receiving message " << msgIn << " from Philosopher ";
//	std::cout << status.Get_source() << std::endl;

	MPI::COMM_WORLD.Recv ( &msgIn, 1, MPI::INT, MPI::ANY_SOURCE, tag, status );
//	std::cout << "Receiving message " << msgIn << " from Philosopher ";
//	std::cout << status.Get_source() << std::endl;	

	//LET'S JUST IGNORE THE MESSAGES AND ASSUME IT'S SAFE TO WRITE TO THE FILE!
    //std::cout << "ID: " << id << " CARELESSLY writing to " << lFile << " and " << rFile << endl;
    //If you want to see correct poems, change MAXMESSAGES to something VERY small and add this sleep
	//sleep(id); //will delay each process so the initial interleaving(s) will likely be OK

    /*                =<< CHECKING FOR AVAILIBILIITY OF FILES >>=                  */
    recv_left = MPI::COMM_WORLD.Iprobe(leftNeighbor, access_file_1);
    recv_right = MPI::COMM_WORLD.Iprobe(rightNeighbor, access_file_2);
    cout << "= [ " << id << ":: CHECK NEIGHBORS ] =" << std::endl;

    if(!recvRight)
    {
      MPI::COMM_WORLD.Send("GOT FILE", 1, MPI::INT, rightNeighbor, access_file_1);
      std::cout << "= [ " << id << ":: ACKNOWLEDGE RECIEVE ] =" << std::endl;
    }
    else
    {
      MPI::COMM_WORLD.Recv(&msgStr, 1, MPI::INT, rightNeighbor, access_file_1, status);
      std::cout << "= [ " << id << ":: RIGHT IN USE ] =" << std::endl;
      std::cout << "= [ " << id << ":: WAIT <- RIGHT ] =" << std::endl;
      MPI::COMM_WORLD.Send("request", 1, MPI::INT, rightNeighbor, file_request_2); // s
      MPI::COMM_WORLD.Send("recieved", 1, MPI::INT, rightNeighbor, access_file_1);
    }
    if(!recv_left)
    {
      MPI::COMM_WORLD.Send("recieved", 1, MPI::INT, leftNeighbor, access_file_2);
      std::cout << "= [ " << id << ":: ACKNOWLEDGE RECIEVE ] =" << std::endl;
    }
    else
    {
      MPI::COMM_WORLD.Recv(&msgStr, 1, MPI::INT, leftNeighbor, access_file_2, status);
      std::cout << "= [ " << id << ":: LEFT IN USE ] =" << std::endl;
      std::cout << "Philo " << id << ":: WAIT <- LEFT ] =" << std::endl;
      MPI::COMM_WORLD.Send("request", 1, MPI::INT, leftNeighbor, file_request_1); // s
      MPI::COMM_WORLD.Send("recieved", 1, MPI::INT, leftNeighbor, access_file_2);
    }
    std::cout << "= [ " << id << ":: ACCESS RECIEVED ] =" << endl;

    /*                         =<< PROCEED WITH WRITING >>=                        */

    //construct poem & output stanzas into the files 'simultaneously'
  //we do this with an intermediate variable so both files contain the same poem!
  string stanza1, stanza2, stanza3;
    stanza1 = P.getLine();
  foutLeft << stanza1 << endl;
    foutRight << stanza1 << endl;

  stanza2 = P.getLine();
  foutLeft << stanza2 << endl;
    foutRight << stanza2 << endl;

  stanza3 = P.getLine();
  foutLeft << stanza3 << endl << endl;
    foutRight << stanza3 << endl << endl;
    
    cout << " =<< " << id << ":: WORK COMPLETE >>=" << endl;

    numWritten++;
  }

  requestRight = MPI::COMM_WORLD.Irecv(&msgStr, 1, MPI::INT, rightNeighbor, file_request_2);
  MPI_Test(&requestRight, &flag, &statusRight);
  if(flag)
  {
    std::cout << "= [ " << rightNeighbor << ":: LEFT WAITING ] = " << endl;
    MPI::COMM_WORLD.Ssend(&id, 1, MPI::INT, rightNeighbor, give_file);
  }
  else
  {
    std::cout << "=<< LEFT:: NO WAIT >>=" << endl;
    MPI_Cancel(&requestRight);
  }

  requestLeft = MPI::COMM_WORLD.Irecv(&msgStr, 1, MPI::INT, leftNeighbor, file_request_1);
  MPI_Test(&requestLeft, &flag, &statusLeft);
  if(flag)
  {
    std::cout << "= [ " << leftNeighbor << ":: LEFT WAITING ] = " << endl;
    MPI::COMM_WORLD.Ssend(&id, 1, MPI::INT, leftNeighbor, give_file);
  }
  else
  {
    std::cout << "=<< LEFT:: NO WAIT >>=" << endl;
    MPI_Cancel(&requestLeft);
  }

  foutLeft.close();
  foutRight.close();
  
  //  Terminate MPI.
  MPI::Finalize ( );
  return 0;
}
