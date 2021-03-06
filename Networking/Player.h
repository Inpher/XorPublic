// (C) 2016 University of Bristol. See License.txt

#ifndef _Player
#define _Player

/* Class to create a player, for KeyGen, Offline and Online phases.
 *
 * Basically handles connection to the server to obtain the names
 * of the other players. Plus sending and receiving of data
 *
 */

#include <vector>
#include <set>
#include <iostream>
#include <fstream>
using namespace std;

#include "Tools/octetStream.h"
#include "Networking/sockets.h"
#include "Networking/ServerSocket.h"
#include "Tools/sha1.h"
#include "Networking/Receiver.h"
#include "Networking/Sender.h"

/* Class to get the names off the server */
class Names
{
  vector<string> names;
  int nplayers;
  int portnum_base;
  int player_no;

  void setup_names(const char *servername);

  void setup_server();

  public:

  mutable ServerSocket* server;

  // Usual setup names
  void init(int player,int pnb,const char* servername);
  Names(int player,int pnb,const char* servername)
    { init(player,pnb,servername); }
  // Set up names when we KNOW who we are going to be using before hand
  void init(int player,int pnb,vector<octet*> Nms);
  Names(int player,int pnb,vector<octet*> Nms)
    { init(player,pnb,Nms); }
  void init(int player,int pnb,vector<string> Nms);
  Names(int player,int pnb,vector<string> Nms)
    { init(player,pnb,Nms); }
  // Set up names from file -- reads the first nplayers names in the file
  void init(int player, int nplayers, int pnb, const string& hostsfile);
  Names(int player, int nplayers, int pnb, const string& hostsfile)
    { init(player, nplayers, pnb, hostsfile); }
  

  Names() : nplayers(-1), portnum_base(-1), player_no(-1), server(0) { ; }
  Names(const Names& other);
  ~Names();

  int num_players() const { return nplayers; }
  int my_num() const { return player_no; }
  const string get_name(int i) const { return names[i]; }
  int get_portnum_base() const { return portnum_base; }

  friend class PlayerBase;
  friend class Player;
  friend class TwoPartyPlayer;
};


class PlayerBase
{
protected:
  int player_no;

public:
  PlayerBase(const Names& Nms) : player_no(Nms.my_num()) {}
  int my_num() const { return player_no; }
};


class Player : public PlayerBase
{
protected:
  vector<int> sockets;
  int send_to_self_socket;

  void setup_sockets(const vector<string>& names,int portnum_base,int id_base,ServerSocket& server);

  int nplayers;

  mutable blk_SHA_CTX ctx;

  map<int,int> socket_players;

  int socket_to_send(int player) const { return player == player_no ? send_to_self_socket : sockets[player]; }

public:
  // The offset is used for the multi-threaded call, to ensure different
  // portnum bases in each thread
  Player(const Names& Nms,int id_base=0);

  virtual ~Player();

  int num_players() const { return nplayers; }
  int socket(int i) const { return sockets[i]; }

  // Send/Receive data to/from player i 
  // 8-bit ints only (mainly for testing)
  void send_int(int i,int a)  const    { send(sockets[i],a);    }
  void receive_int(int i,int& a) const { receive(sockets[i],a); }

  // Send an octetStream to all other players 
  //   -- And corresponding receive
  virtual void send_all(const octetStream& o,bool donthash=false) const;
  void send_to(int player,const octetStream& o,bool donthash=false) const;
  virtual void receive_player(int i,octetStream& o,bool donthash=false) const;

  // Receive one from player i

  /* Broadcast and Receive data to/from all players 
   *  - Assumes o[player_no] contains the thing broadcast by me
   */
  void Broadcast_Receive(vector<octetStream>& o,bool donthash=false) const;

  /* Run Protocol To Verify Broadcast Is Correct
   *     - Resets the blk_SHA_CTX at the same time
   */
  void Check_Broadcast() const;

  // wait for available inputs
  void wait_for_available(vector<int>& players, vector<int>& result) const;

  // dummy functions for compatibility
  virtual void request_receive(int i, octetStream& o) const { sockets[i]; o.get_length(); }
  virtual void wait_receive(int i, octetStream& o, bool donthash=false) const { receive_player(i, o, donthash); }
};


class ThreadPlayer : public Player
{
public:
  mutable vector<Receiver*> receivers;
  mutable vector<Sender*>   senders;

  ThreadPlayer(const Names& Nms,int id_base=0);
  virtual ~ThreadPlayer();

  void request_receive(int i, octetStream& o) const;
  void wait_receive(int i, octetStream& o, bool donthash=false) const;
  void receive_player(int i,octetStream& o,bool donthash=false) const;

  void send_all(const octetStream& o,bool donthash=false) const;
};


class TwoPartyPlayer : public PlayerBase
{
private:
  // setup sockets for comm. with only one other player
  void setup_sockets(const char* hostname, ServerSocket& server, int pn, int id);

  int socket;
  bool is_server;
  int other_player;

public:
  TwoPartyPlayer(const Names& Nms, int other_player, int pn_offset=0);
  ~TwoPartyPlayer();

  void send(octetStream& o) const;
  void receive(octetStream& o) const;

  int other_player_num() const;

  /* Send and receive to/from the other player
   *  - o[0] contains my data, received data put in o[1]
   */
  void send_receive_player(vector<octetStream>& o) const;
};

#endif
