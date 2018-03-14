/*
 *      AUTHORS: Matthew Healy <mhealy@mst.edu>
 *               Cody Roberts <cargy9@mst.edu>
 *
 *       COURSE: CS3800 - Intro. to Operating Systems
 *   INSTRUCTOR: Michael Gosnell <gosnellm@mst.edu>
 *  DESCRIPTION: An impementation of the "Dining Philosophers" problem,
 *               shown as philosophers who write to two files simultaneously.
 */

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
const int MAX_MESSAGES = 2; 

//if you change this base, update the Makefile "clean" accordingly to remove old poems
const string fileBase = "outFile"; 

int main ( int argc, char *argv[] ) 
{
  int id; //my MPI ID
  int p;  //total MPI processes
  MPI::Status status_left;
  MPI::Status status_right;
  MPI::Status status_glob;
  int give_file = 1;
  int tag = 4;

  MPI::Init(argc, argv);
  p = MPI::COMM_WORLD.Get_size();
  id = MPI::COMM_WORLD.Get_rank();

  if (p < 2) {
      MPI::Finalize();
      std::cerr << "= [ 2 OR MORE PHILOSOPHERS REQUIRED ] =" << std::endl;
      return 1;
  }

  srand(id + time(NULL));

  int numWritten = 0;
  bool has_left_file = false;
  bool has_right_file = false;
  int msg_in, msg_out;
  int left_neighbor = (id - 1 + p) % p;
  int right_neighbor = (id + 1) % p;
  pomerize P;

  string left_file = fileBase + to_string(left_neighbor) + ":" + to_string(id);
  string right_file = fileBase + to_string(id) + ":" + to_string(right_neighbor);
  ofstream fout_left(left_file.c_str(), ios::out | ios::app );
  ofstream fout_right(right_file.c_str(), ios::out | ios::app );
  if (id < p - 2) {
    has_right_file = true;
  }
  if (id == p - 1) {
    has_left_file = true;
    has_right_file = true;
  }


  while (numWritten < MAX_MESSAGES) {
    MPI::Request req_left;
    MPI::Request req_right;
    MPI::Request rec_left;
    MPI::Request rec_right;
    /*                 =<< STARTING RANDOM PROCESS >>=     */
    msg_out = rand() % p;
    MPI::COMM_WORLD.Send (&msg_out, 1, MPI::INT, left_neighbor, tag);
    msg_out = rand() % p;
    MPI::COMM_WORLD.Send (&msg_out, 1, MPI::INT, right_neighbor, tag);
    MPI::COMM_WORLD.Recv (&msg_in, 1, MPI::INT, MPI::ANY_SOURCE, tag, status_glob);
    /*                  =<< BEGIN PROCESS LOGIC >>=       */

    /*                =<< CHECKING FOR AVAILIBILIITY OF FILES >>=                  */

    // Check availability of files
    if (!has_left_file) {
      cout << "= [ " << id << " :: WAITING FOR LEFT FILE :: " << left_neighbor << "] =" << endl;
      MPI::COMM_WORLD.Recv(&msg_in, 1, MPI::INT, left_neighbor, give_file);
      cout << "= [ " << id << " :: LEFT FILE RECIEVED ] =" << endl;
    }

    if (!has_right_file) {
      cout << "= [ " << id << " :: WAITING FOR RIGHT FILE :: " << right_neighbor << " ] =" << endl;
      MPI::COMM_WORLD.Recv(&msg_in, 1, MPI::INT, right_neighbor, give_file);
      cout << "= [ " << id << " :: RIGHT FILE RECIEVED ] =" << endl;
    }

    //construct poem & output stanzas into the files 'simultaneously'
    //we do this with an intermediate variable so both files contain the same poem!
    
    cout << "\n=============================================" << endl;
    cout << "||  =<< " << id << " :: BEGIN CRITICAL SECTION "<< numWritten+1 << " >>=  ||" << endl;
    cout << "=============================================\n" << endl;
    string stanza_1, stanza_2, stanza_3;
    stanza_1 = P.getLine();
    fout_left << stanza_1 << endl;
    fout_right << stanza_1 << endl;

    stanza_2 = P.getLine();
    fout_left << stanza_2 << endl;
    fout_right << stanza_2 << endl;

    stanza_3 = P.getLine();
    fout_left << stanza_3 << endl << endl;
    fout_right << stanza_3 << endl << endl;
    
    cout << "\n====================================" << endl;
    cout << "||  =<< " << id << " :: WORK COMPLETE " << numWritten+1 <<" >>=  ||" << endl;
    cout << "====================================\n" << endl;
    numWritten++;
  
  /*                              =<< GIVE BACK CONTROL >>=                           */
    has_left_file = false;
    MPI::COMM_WORLD.Send(&msg_out, 1, MPI::INT, left_neighbor, give_file);
    cout << "= [ " << id << " :: RETURNING LEFT ] =" << endl;

    has_right_file = false;
    MPI::COMM_WORLD.Send(&msg_out, 1, MPI::INT, right_neighbor, give_file);
    cout << "= [ " << id << " :: RETURNING RIGHT ] =" << endl;
  }
  cout << "\n=========================================" << endl;
  cout << "||  =<< " << id << " :: ALL MESSAGES WRITTEN >>=  ||" << endl;
  cout << "=========================================\n" << endl;
  fout_left.close();
  fout_right.close();

  MPI::Finalize();
  return 0;
}
