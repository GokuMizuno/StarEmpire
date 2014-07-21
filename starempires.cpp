/* v1.0
Star-Empires, author D. Bolton (C)2011 http://cplus.about.com
This is from the series of Games Programming in C++ by David Bolton
Article url: 
Last updated: 10 Oct 2011 
*/

#include <time.h> 
#include <iostream>
#include <sstream>
#include <string>
#include <conio.h> // needed for _getch() 
#include <iomanip>
#include <cmath>
#include <Windows.h>
#include <vector>

// game data 
const int  width= 80;
const int  height= 50;
const int  maxstarsystems= 10;
const int  fightmarker= 999;
const int  SecsPerTurn= 5;
const int  msgline= 25;
const int  cmdline= 45;
const int  maxinputlen=4;

// data structures  
struct fleet {
	int from;
	int to;
	int turns;
	int numships;
	bool infight;
	int owner; // (o or 1) 
    fleet() : infight(false) {;}
};

struct starsystem {
	int x,y;
	int numships;
	int owner;
	void addships(int numtoadd);
	void removeships(int numtoremove);
	starsystem(int i) : x(0),y(0),owner(i),numships(0){;}
};

// display class for clear screen etc
class display {
private:	
	COORD GetConsoleSize();
	COORD SetScreenSize(short x,short y);
	void ClrScr(); 
public:
	void gotoxy( short x, short y );
	void clr(int line);
	display ();
};


class game; // forward declaration

// This handles all messages including storage and display
class messageHandler {
	std::string messagebuffer;
    std::vector<std::string> messages;
    std::string spaces;
	display & crt;
	game & Game;
public:
	messageHandler(display & _crt,game & _game) : crt(_crt), Game(_game){
		spaces.assign(66,' ');
	}
	void bmsg(std::string);
	void bmsg2(std::string, int value);
	bool CheckMessages();
	const std::string & buffer() {return messagebuffer;}
	void DisplayTimedMessage(int line);
	void PrintMsg(char * msgbuffer) {messagebuffer = msgbuffer;}
};

// Bulk of functionality is here
class galaxy {
private:
  std::vector<starsystem> starsystems;
  std::vector<fleet> fleets;
  void Initstarsystem(int starsystemindex,int x,int y,int shipnums,int owner) ;
  display & crt;
  messageHandler & m;
  game & Game;
  int shipsSent(int tostarsystem);
  int turnstogo(int tostarsystem);
  char layout[5][5];
  void retreat(int playernum,int astarsystem);
  void RemoveCasualties(int playernum,int casualties,int atstarsystem);
  int doOneonOne(int fleet0,int fleet9,int atstarsystem);
  void CheckGameOver();
  void SendHomeFleets(int playernum,int atstarsystem);
  int Attackstarsystem(int playernum,int fleetp, int atstarsystem);
  int findNeareststarsystem(int atstarsystem);
  int fight(int fleet0,int fleet9,int atstarsystem);
  void MoveFleetToOrbit(int atstarsystem);  
public:  
  galaxy(display & _display,messageHandler & _m,game & _game);
  void GenMapstarsystems();
  int getstarsystem(int x,int y);
  char owner(int x,int y);
  char owner(int systemindex);
  int getNumShips(int systemindex,int playernum);
  int getNumShips(int systemindex) {return starsystems[systemindex].numships;}
  void DisplayMap();
  void DisplayDistances();
  void DisplayFleets();
  void DisplayMessageArea();
  int  AddFleet(int player,int numships,int source,int target);
  void BuildFleets();
  void BuildFleetsAtAllstarsystems(int numships,int target);
  int MoveFleets();
  void EnemyOrders();
  void DoFights();
};

// High level function manages game setup and turns
class game {
private:
	int turn;
	display crt;
	messageHandler m;
	galaxy Galaxy;
	void InitData();
    void Process_Command(char c);
public:	
	bool GameOver;
	game() : m(crt,*this), Galaxy(crt,m,*this)  { InitData(); 	}
    void DoTurn();
    void GetandProcessOrders();
	int Turn(){return turn;}
	bool ScrollMessages() {return m.CheckMessages();}
	void showmsg(std::string msg) {m.bmsg(msg);}
};


clock_t lasttick;

// functions that are global

int Random(int max) { return (rand() % max) + 1;}

// returns distance as int. Values are always in the range 1-6 
int distance(int x,int y) {
   int distsqr= (x*x) + (y*y);
   return (int)(sqrt((float)distsqr)+0.5);
}

// Takes an int and formats it to specified width
std::string toWidth(int value, int width)
{
    std::ostringstream ss;
    ss << std::fixed << value;
	ss.width(width);
    return ss.str();
}

// wait for numms (millisecs) 
void waitms(int numms) {
	clock_t until;
	until = clock()+(numms*1000/CLOCKS_PER_SEC);	// 1,2,3...		
	while (clock() < until) {
		if (_kbhit())
			break;
	}
}

// return 1 if one second passed since last lasttick 
int onesecpassed() {
	clock_t tick = clock();
	if (tick-lasttick > CLOCKS_PER_SEC)
		{
			lasttick = tick;
			return 1;
	}
	else
		return 0;
}

// handles input chars 0-9, backspace and return 
int GetInput() {
	std::string back ="\b \b";
	std::string buffer; // max 4 digits 
	char key;
	int result=0;
	int buflen=0;
	int hitreturn=0;
	do {
		key=_getch();
		if (key==27)
			return -1;
		buflen=buffer.length();
		if ((buflen < maxinputlen && key >='0' && key <='9') || 
			(buflen >0 && (key== 8 || key==13)))
		{
			  if (key ==13)
				  hitreturn=1;
			  else
				if (key==8)
				{
					buffer.erase(buflen-1);
					printf("%s",back.c_str());
					
				}
				else
				{
					buffer.append(1,key);		
					printf("%c",key);
				}
		}
	}
     while (!hitreturn);
	 return atoi(buffer.c_str());
}

// Class Methods ...........

//               game class

// initializes all the game data in one place 
void game::InitData() {
	GameOver=false;
	srand((int)time(NULL));
	turn=0;
	lasttick = clock();	
	Galaxy.GenMapstarsystems();
	Galaxy.DisplayDistances();
	Galaxy.DisplayMessageArea();
}

// This is processed in the game loop
void game::DoTurn() {
	crt.gotoxy(45,5);	
	m.bmsg2("Turn ",++turn);
	printf("Turn %d",turn);
	Galaxy.DisplayMap();
	Galaxy.DisplayFleets();	
	Galaxy.MoveFleets();
	GetandProcessOrders();
	Galaxy.EnemyOrders();
	if (Galaxy.MoveFleets())
		Galaxy.DoFights();

	Galaxy.BuildFleets();		
}

// This waits for SECSPERTURN seconds for input, 
//and returns if no command in that time. While waiting, if the key isn't pressed
//it calls CheckMessage() to pump the message queue 
void game::GetandProcessOrders() {
	clock_t timeinsecs;
	char c;
	timeinsecs = clock()+(SecsPerTurn*CLOCKS_PER_SEC);	// 5 Seconds initially but change SECSPERTURN 
	crt.gotoxy(0,cmdline);printf("Move Command: Press 0-9 or A(ll):");
	while (clock() < timeinsecs) {

		if (_kbhit())  // Key pressed 
		{
			c = _getch();
			if (c=='\0')
				c=_getch();
			if (c==27)	{
					GameOver=1;
					return;
			}
			
			if ((c>='0' && c <='9') || (c=='a' || c=='A')) {
				{
					
					Process_Command(c);
					crt.gotoxy(0,cmdline);printf("Move Command: Press 0-9 or A(ll): ");
				}
			}
			else
			{
				printf("%c",7);
			}
		}
		else
			m.CheckMessages();
	}
}

// processes the command presed which is 0-9 or a/A 
void game::Process_Command(char c) {
	char t;
	char localbuffer[100];
	int source,target,numships;

	crt.gotoxy(0,cmdline+1);printf("Target starsystem: Press 0-9: ");
	t=_getch();
	printf("%c",t);
	if (t >='0' && t <='9' && t != c) {
		if (c=='a' || c=='A')
			source =99;
		else
			source = (int)(c-'0');
		target = (int)(t-'0');
		if (source !=99 && Galaxy.owner(source)!=0) {
			m.bmsg2("You don't own starsystem ",source);
			m.DisplayTimedMessage(cmdline+2);
			crt.clr(cmdline+1);
			return;		
		}
		crt.gotoxy(0,cmdline+2);printf("How Many Ships:");numships=GetInput();
		if (numships >-1 )
		  {
			  if (source != 99 && numships > Galaxy.getNumShips(source))
			  {
				  sprintf(localbuffer,"Can't send %i ships. Order reduced to %i.",numships,Galaxy.getNumShips(source));
				  m.PrintMsg(localbuffer);
				  m.DisplayTimedMessage(cmdline+3);
				  numships= Galaxy.getNumShips(source,0);
			  }
			  if (source == 99) {
				Galaxy.BuildFleetsAtAllstarsystems(numships,target);
			  }
			  else
			  {
			    sprintf(localbuffer,"Move %d from %d to %d",numships,source,target);
    		    m.DisplayTimedMessage(cmdline+3);
			    Galaxy.AddFleet(0,numships,source,target);
			  }
		}
		crt.clr(cmdline+1);
		crt.clr(cmdline+2);
	}
	
}

// NumShips - add up all ships at a starsystem for a player 
int galaxy::getNumShips(int atstarsystem,int playernum) {
	int i,numships;
	numships=0;
	for (i=0;i<(int)fleets.size();i++) 
		if (fleets[i].infight && fleets[i].to==atstarsystem && 
				fleets[i].owner==playernum )
				numships += fleets[i].numships;
    return numships;
}

//             starsystem class

void starsystem::addships(int numtoadd) {
	numships += numtoadd;
	if (numships > 999)
	{
		numships=999; // maximum cap 
	}
	if (owner==-1 && numships > 250)
	{
		numships=250; // maximum cap 
	}
}

// remove specified number of ships so long as it doesn't exceed
// the actual number of ships
void starsystem::removeships(int numtoremove) {
	if (numtoremove > numships) 
		numtoremove = numships;
	numships -= numtoremove;
}

//             galaxy class 

galaxy::galaxy(display & _display,messageHandler & _m,game & _game) : crt(_display),m(_m),Game(_game) {
	for (int i=0;i<maxstarsystems;i++) {
	    starsystem asystem(i);
		starsystems.push_back(asystem);
	}
}

// This initialises a single starsystem 
void galaxy::Initstarsystem(int starsystemindex,int x,int y,int shipnums,int owner) {
		starsystems[starsystemindex].x=x;		
		starsystems[starsystemindex].y=y;
		starsystems[starsystemindex].numships = shipnums;
		starsystems[starsystemindex].owner = owner;
		layout[x][y]=(char)(starsystemindex+48);
}

// Generates a random layout of starsystems (apart from starsystems 0 and 9 which are always fixed.
//This also sets up the number of ships at aeach starsystem 
void galaxy::GenMapstarsystems() {
	int i,x,y;

	for (x=0;x<5;x++)
	for (y=0;y<5;y++) {
		layout[x][y]=' ';
	}

	Initstarsystem(0,0,0,50,0);
	Initstarsystem(9,4,4,50,9);

	// Find an empty space for remaining 8 starsystems
	for (i=1;i<=8;i++) {
		do {
			x= Random(5)-1;
			y= Random(5)-1;
		}
		while (layout[x][y] !=' ');
	   Initstarsystem(i,x,y,15,-1);
	}

}

// returns the index in the galaxy for the sysm at x,y 
int galaxy::getstarsystem(int x,int y) {
	int i;
	for (i=0;i<(int)starsystems.size();i++)
		if (starsystems[i].x==x && starsystems[i].y==y){
			return i;
		}
	return -1; // should never get here! 
}

// returns a + for starsystems owned by the player, - for enemy and u for unowned 
char galaxy::owner(int x,int y) {
	int i= getstarsystem(x,y);
	if (i != -1)
	  {
		if (starsystems[i].owner==0)
			return '+';
		else
			if (starsystems[i].owner==9)
			return '-';
			else
			return 'u';
		}
	return ' '; // should never get here! 
}

char galaxy::owner(int systemindex) { return starsystems[systemindex].owner;}

void galaxy::DisplayMap() {
int x,y;
  crt.gotoxy(0,5);printf("Map\n\r");
for (y=0;y<5;y++) {
	printf("   ");
	for (x=0;x<5;x++) {
		if (layout[x][y]==' ')
			printf(" .  ");
		else
			printf(" %c%c ",layout[x][y],owner(x,y));
	}
	
	printf("\n\r");
   }
}

void galaxy::DisplayDistances() {
int x,y,sx,sy,sx2,sy2;
crt.gotoxy(0,12);printf("starsystem Distances\n\r");
printf("  To   1  2  3  4  5  6  7  8  9\n\r");
printf("     ----------------------------\n\r");
for (y=0;y<9;y++) {
	printf("      ");
	sx = starsystems[y].x;
	sy = starsystems[y].y;
	for (x=1;x<10;x++) {
	    if (x>y)
		{
			sx2 = starsystems[x].x;
			sy2 = starsystems[x].y;
			printf(" %c ",(char)(distance(sx-sx2,sy-sy2)+'0'));
		 }
		else
			printf("   ");
	}
	printf("| %2d\n\r",y);
   }
}

void galaxy::DisplayFleets() {
	int i,owner,ttg;
	std::string text;
	crt.gotoxy(45,13);printf("starsystem(Own) Ships Sent(Turns)");
	for (i=0;i<(int)starsystems.size();i++) {
	  crt.gotoxy(45,14+i);
	  owner = starsystems[i].owner;
	  if (owner==-1 )
		text="u";
	  else
		 text = toWidth(owner,5);
	  int numships = starsystems[i].numships;
	  int shipssent = shipsSent(i);
	  printf("%2i (%5s)    %3i   %3i",i,text.c_str(),numships,shipssent);
	  ttg = turnstogo(i);
	  if (ttg!= 99)
		  printf("(%i)  ",ttg); 
	  else
		  printf("     ");
	}
}

// totals up how many player's ships are on the way to the specified starsystem 
int galaxy::shipsSent(int tostarsystem) {
	int i,total;
	total=0;
	for (i=0;i<(int)fleets.size();i++)
		if (fleets[i].to==tostarsystem && fleets[i].numships >0 && fleets[i].owner==0)
			total+= fleets[i].numships;
	return total;
}

// calculates the shortest arrival time of any fleet at a starsystem 
int galaxy::turnstogo(int tostarsystem) {
	int i,ttg;
	ttg = 99;
	for (i=0;i<(int)fleets.size();i++)
	{
		if (fleets[i].to==tostarsystem && fleets[i].numships >0 && fleets[i].owner==0)
			{
				if (fleets[i].turns<ttg)
					ttg= fleets[i].turns;
				else
					if (fleets[i].infight)
						ttg = 0;
		}
	}
	return ttg;
}

// Boxes off the area for messages to appear in 
void galaxy::DisplayMessageArea(){
  crt.gotoxy(0,msgline);printf(   "============================= Messages =============================");
  crt.gotoxy(0,msgline+18);printf("====================================================================");
}

// add a new fleet from system source
int galaxy::AddFleet(int player,int numships,int source,int target) {
	int sx,sy,tx,ty;
	starsystem & fromsystem= starsystems[source];
	sx = fromsystem.x;
	sy = fromsystem.y;
	if (numships > fromsystem.numships)
		numships = fromsystem.numships;

	starsystem & tosystem = starsystems[target];
	tx = tosystem.x;
	ty = tosystem.y;	

	fleet  afleet;
	afleet.from=source;
	afleet.to=target;
	afleet.turns = distance(sx-tx,sy-ty);
	afleet.owner=player;
	afleet.numships = numships;
	afleet.infight=false;
	fleets.push_back(afleet);
	fromsystem.removeships(numships);
	return 1;
}

// adds new ships at each starsystem. Unowned system are capped at 250, owned at 999 
//This is called once per turn 
void galaxy::BuildFleets() {
	int i,numshipsbuilt;
	for (i=0;i<(int)starsystems.size();i++)
	{
		starsystem & asystem = starsystems[i];
		numshipsbuilt = asystem.numships/10;	
		if (numshipsbuilt==0)  // always build 1 ship 
			numshipsbuilt=1;
		if (numshipsbuilt > 15)
			numshipsbuilt=15;
		asystem.addships(numshipsbuilt);
	}
}

// if a/A was pressed, this assembles fleets at all starsystems and sends them to the target,
//  it builds up each fleet one ship at a time so we don't try get them all from one starsystem
void galaxy::BuildFleetsAtAllstarsystems(int numships,int target) {
	int anyleft,i;
	int fleetsize[maxstarsystems];

	for (i=0;i<maxstarsystems;i++)
		fleetsize[i]=0;
	while (numships>0 ) {
		anyleft =0;
		for (i=0;i<maxstarsystems;i++) {
			if (starsystems[i].owner==0 && starsystems[i].numships >0)
			{
				fleetsize[i]++;
				starsystems[i].numships--;
				anyleft += starsystems[i].numships;	
				numships--;
			}
		}
		if (anyleft==0)
			break;
	}
	for(i=0;i<maxstarsystems;i++) 
		if (fleetsize[i]>0) {
			starsystems[i].numships= fleetsize[i]; // needed as AddFleet deducts ships 
			AddFleet(0,fleetsize[i],i,target);
		}
}

// transfers any ship fleets that have arrived at a friendly starsystem and add them to 
//starsystem defences 
void galaxy::MoveFleetToOrbit(int atstarsystem) {

	int i,playernum;
	playernum= starsystems[atstarsystem].owner;
	for (i=0;i< (int)fleets.size();i++) {
		if (fleets[i].to==atstarsystem && fleets[i].owner==playernum && fleets[i].numships>0)
		{
			starsystems[atstarsystem].numships += fleets[i].numships;
			fleets[i].turns=0;
			fleets[i].numships=0;
			fleets[i].to=0;
		}
	}
}

// called once per turn  
int galaxy::MoveFleets() {
	int destination;
	int numfights =0;
	for (size_t i=0;i<(int)fleets.size();i++) {
		if (fleets[i].numships >0) {
			{
				fleets[i].turns--;
				if (fleets[i].turns==0) { // arrived 
					destination = fleets[i].to;
					if (starsystems[destination].owner== fleets[i].owner) 
					{  // we own starsystem so reinforce it 
						MoveFleetToOrbit(destination);
					}
					else
					{// fight, fight, fight! 
						fleets[i].infight=true; 
						numfights++;
					}
				}
			}
		}
	}
	return numfights;
}

// Simple AI. Send half from any starsystem over 30 strong to nearest starsystem 
void galaxy::EnemyOrders() {
	int numships,target;
	for (int i=0;i<maxstarsystems;i++)
		if (starsystems[i].owner==9 && starsystems[i].numships > 30) {
			target = findNeareststarsystem(i);
			if (target != -1)
			{
				numships = starsystems[i].numships/2;
				starsystems[i].numships -= numships;
				AddFleet(9,numships,i,target);
				m.bmsg2("Enemy launched fleet to starsystem ",target);
			}
		}
}

//                   MesageHandler Class
// display a message for a second then remove it 
void messageHandler::DisplayTimedMessage(int line) {
		crt.gotoxy(0,line);
		std::cout << messagebuffer;
		waitms(1000); // waits 1000 ms, ie 1 sec 
		crt.clr(line);
}


// messages go in a 100 x 100 message buffer array with msgtail and msghead
// messages are added using the tail and displayed from the msghead 
void messageHandler::bmsg(std::string text) {
	char localbuffer[100];
	sprintf(localbuffer,"%i %s",Game.Turn(),text.c_str());
	messages.push_back(localbuffer);
}

// writes a message with a single int parameter 
void messageHandler::bmsg2(std::string msgtext,int x) {
  char buffer[100];
  sprintf(buffer,"%s %i",msgtext.c_str(),x);
  bmsg(buffer);
}

//                    Display class
// moves the cursor to x,y x- across, y = down 
void display::gotoxy( short x, short y ) 

{ 
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE) ; 
    COORD position = { x, y } ; 
     
    SetConsoleCursorPosition( hStdout, position ) ; 
}  

COORD display::GetConsoleSize() 
{ 
    HANDLE hnd = GetStdHandle(STD_OUTPUT_HANDLE) ; 
    return GetLargestConsoleWindowSize(hnd); 
}  

COORD display::SetScreenSize(short x,short y) {
	COORD result; 
	HANDLE hnd;
	result.X=0;
	result.Y=0;
    hnd= GetStdHandle(STD_OUTPUT_HANDLE) ; 
    SetConsoleDisplayMode( hnd,CONSOLE_WINDOWED_MODE,&result);
    return result;
}

display::display () {
	COORD scrSize;	
	scrSize=SetScreenSize(width,height);	
	ClrScr();
}
// clears the screen area 
void display::ClrScr() {
	COORD c;
	DWORD NumWr;
	HANDLE hnd = GetStdHandle(STD_OUTPUT_HANDLE) ;
	c.X=0;
	c.Y=0;
     
    FillConsoleOutputCharacter(hnd,' ',width*height,c,&NumWr);
	// FillConsoleOutputAttribute(hnd,0x88,4000,c,&NumWr);  for setting color
}

// clears line# in the display 
void display::clr(int line) {
	std::string s="";
	s.resize(width,' ');
	gotoxy(0,line);	
	std::cout << s;
}



void galaxy::CheckGameOver() {
	int i,countpl,countenemy,checkfleets;
	checkfleets=0;
	countpl=0;
	countenemy=0;
	for (i=0;i<maxstarsystems;i++)
		if (starsystems[i].owner==0)
			countpl++;
		else
		if (starsystems[i].owner==9)
			countenemy++;
	if (countpl==maxstarsystems)
		{
			m.bmsg("You have captured ten starsystems");
			checkfleets=1;
	}
	else
	if (countenemy==maxstarsystems)
		{
			m.bmsg("Enemy has captured ten starsystems");
			checkfleets=1;
	}
	
	if (countpl==0) {
			m.bmsg("You have lost all your starsystems");
			checkfleets=1;	
	}
	
	if (countenemy==0) {
			m.bmsg("Enemy has lost all their starsystems");
			checkfleets=1;	
	}	

	if (checkfleets) { // Check if any fleets in flight, if none, game over! 
		checkfleets=0;
		for (i=0;i<(int)fleets.size();i++)
			if (fleets[i].turns >0 && fleets[i].numships >0)
				checkfleets++;
		if (checkfleets==0)
		{
			if (countpl==maxstarsystems || countenemy==0)
				{
					m.bmsg("You have Won!");
					Game.GameOver = true;
			}
			else
				if (countenemy == maxstarsystems || countpl ==0) {
				    m.bmsg("The Enemey player has won. Tough luck");
					Game.GameOver=true;
				}
		}

	}
}

// forces playernum (0 or 9) to retreat to starsystem they came from 
void galaxy::retreat(int playernum,int astarsystem) {
	int temp,from,xf,yf,xt,yt;
	for (size_t i=0;i< fleets.size();i++) {
		if (fleets[i].numships >0 && fleets[i].infight && 
			fleets[i].to==astarsystem && fleets[i].owner==playernum) {
			if (fleets[i].numships >0) {
				temp = fleets[i].to;
				from=fleets[i].from;
				fleets[i].to=from;
				fleets[i].from = temp;
				xt = starsystems[temp].x;
				yt = starsystems[temp].y;
				xf = starsystems[from].x;
				yf = starsystems[from].y;
				fleets[i].turns=distance(xt-xf,yt-yf);
			}
		}
	}
}

// This removes one ship at a time from all fleets in battles at a starsystem for one player 
void galaxy::RemoveCasualties(int playernum,int casualties,int atstarsystem) {
	char localbuffer[100];
	int f0,f9,fs,totalships;
	f0=0;
	f9=0;
	fs=0;
	while (casualties>0) {
		totalships =0;
		for (size_t i=0;i<fleets.size();i++) {
			if (fleets[i].infight && fleets[i].to==atstarsystem && 
				fleets[i].owner==playernum && fleets[i].numships >0)
					{
						--casualties;
						--fleets[i].numships;
						if (fleets[i].numships<=0)
						{							
							if (playernum==0) {
								sprintf(localbuffer,"Fleet %i lost, attacking starsystem %i",i,fleets[i].to);
								m.PrintMsg(localbuffer);
							}
							fleets[i].to=0;
						    fleets[i].turns=0;		
							fleets[i].numships=0;

						}
						totalships += fleets[i].numships;
					}
			}	// for 	
		if (totalships==0)
			break;
	}
}

int galaxy::doOneonOne(int fleet0,int fleet9,int atstarsystem) {
int p0,p9,cas0,cas9,tcas0,tcas9,f0,f9,factor,i;
  p0=50;
  p9=50;
  if (fleet0>fleet9) 
	{
		factor = fleet0/fleet9;
		if (factor >= 2) p0 +=10;
		if (factor >= 3) p0 +=10;
		if (factor >= 4) p0 +=10;    
    }
  else
  {
		factor = fleet9/fleet0;
		if (factor >= 2) p9 +=10;
		if (factor >= 3) p9 +=10;
		if (factor >= 4) p9 +=10;    
  }
  tcas0=0;
  tcas9=0;
  f0=fleet0;
  f9=fleet9;
  while (fleet0 >0 && tcas0*100/f0 <50 && fleet9 >0 && tcas9*100/f9 < 50) 
  {
	cas9 =0;
	for (i=0;i<fleet0;i++) 
		if (Random(100)<= p0)
			cas9++;
	cas0 =0;
	for (i=0;i<fleet9;i++) 
		if (Random(100)<= p9)
			cas0++;
    tcas0 += cas0;
	tcas9 += cas9;
	fleet0 -=cas0;
	fleet9 -=cas9;
	if (fleet0<0) fleet0=0;
	if (fleet9<0) fleet9 =0;
  }	
  RemoveCasualties(0,tcas0,atstarsystem);
  RemoveCasualties(9,tcas9,atstarsystem);
  if (tcas0*100/f0>50)
	  return 9; // - means 0 won 9 lost 
  else
	  return  0; // + means 9 won 0 lost 
}



// 
void galaxy::SendHomeFleets(int playernum,int atstarsystem) {
	int tx,ty,hx,hy,home;
	char localbuffer[100];
	for (size_t i=0;i<fleets.size();i++) 
		if (fleets[i].infight && fleets[i].to==atstarsystem && 
			fleets[i].owner==playernum && fleets[i].numships>0)
		{		
			
			home=fleets[i].from;
			tx=starsystems[atstarsystem].x;
			ty=starsystems[atstarsystem].y;			
			hx=starsystems[home].x;
			hy=starsystems[home].y;
			fleets[i].to= home;
			fleets[i].from= atstarsystem;
			fleets[i].turns= distance(tx-hx,ty-hy);
			sprintf(localbuffer,"  Fleet %i lost against starsystem %i.  %i ships back to starsystem %i",i,
				atstarsystem,fleets[i].numships,home);
			m.PrintMsg(localbuffer);
		}
}

// This handles a fight at a starsystem between a player (0 or 9)
// and the starsystem fletp = nums of ships 
int galaxy::Attackstarsystem(int playernum,int fleetp, int atstarsystem) {
	int pp,ps,casp,cass,tcass,tcasp,factor,i,fleetsys;
  char localbuffer[100];
  pp=50; // Prob% of player's ships destroying starsystem  ships 
  ps=60; // Prob% of starsystem's ships destroying player's ship 
  fleetsys= starsystems[atstarsystem].numships;
  if (fleetsys >0 && fleetp>fleetsys)  // work out odds improvement 
	{
		factor = fleetp/fleetsys;
		if (factor >= 2) pp +=10; // 2:10 + 10% 
		if (factor >= 3) pp +=10; // 3:1  + 10% 
		if (factor >= 4) pp +=10; // 4:1  + 10%    
    }
  else
  {
		factor = fleetsys/fleetp;
		if (factor >= 2) ps +=10;
		if (factor >= 3) ps +=10;
		if (factor >= 4) ps +=10;    
  }
  tcasp=0; // total cas of player 
  tcass=0; // total cas of starsystem 

  // fight continues while players fleet is not destroyed (fleetp != 0)
  // and players fleet has less than 50% casualties and starsystem ships not destroyed 
  while (fleetp > 0 && (tcasp*100/fleetp <50) && (fleetsys >0)) 
  {
	cass =0;
	for (i=0;i<fleetp;i++) 
		if (Random(100)<= pp)
			cass++; // how many starsystem ships desytoyed this round 
	casp =0;
	for (i=0;i<fleetsys;i++) 
		if (Random(100)<= ps) // how many players ships desytoyed this round 
			casp++;
    tcasp += casp; // add this rounds casulaties to total (player)
	tcass += cass; // add this rounds casulaties to total (player)
	fleetp -= casp; // deduct casualties from players ships  
	fleetsys -= cass; //  deduct casualties from starsystems ships  
	if (fleetp<0) // if negative make zero 
		fleetp=0;
	if (fleetsys<0) // if negative make zero 
		fleetsys=0;
  }	
  RemoveCasualties(playernum,tcasp,atstarsystem); // remove players casualties from all fleets at syste, 
  starsystems[atstarsystem].numships -= tcass;    // remove starsystem casulaties 
  if (starsystems[atstarsystem].numships<0)   // if negative make zero 
	  starsystems[atstarsystem].numships =0;   
  if (starsystems[atstarsystem].numships==0) // starsystem captured if no ships left 
  {
	  starsystems[atstarsystem].owner= playernum;
	  MoveFleetToOrbit(atstarsystem);  // disband fleets, now starsystem ships  
	  CheckGameOver();
	  if (playernum==0) 
		  {
			  m.bmsg2("You captured starsystem ",atstarsystem);
			  return 0;
	  }
	  else
	  {
		  m.bmsg2("Enemy captured starsystem ",atstarsystem);
		  return 9;
	  }
  }
  {
	  if (playernum==0) 
		  m.bmsg2("Enemy failed to capture starsystem ",atstarsystem);
	   else
		  m.bmsg2("Enemy failed to capture starsystem ",atstarsystem);
	  sprintf(localbuffer,"starsystem %i lost %i ships",atstarsystem,tcass);
	  m.PrintMsg(localbuffer);
	  SendHomeFleets(playernum,atstarsystem);
  }
  return -1;
}

//
//Possible three way fight if player and enemy fleets arrive on same turn.
//if fleet0 and fleet9 have ships then they fight, winner then attack the starsystem
//if only one of fleet0 or Fleet9 present then they fight fleet starsystem


int galaxy::fight(int fleet0,int fleet9,int atstarsystem) {
	int lostpl;
	if (fleet0 >0 && fleet9 >0) {
		lostpl=doOneonOne(fleet0,fleet9,atstarsystem); // winner is 0 or 9 
		retreat(lostpl,atstarsystem);		
		m.bmsg2("You fought the enemy at starsystem ",atstarsystem);
		if (lostpl==0)
			{
				fleet0=0;
				m.bmsg("   You lost ");
				fleet9 = getNumShips(atstarsystem,9);
		}
		else
			{
				fleet9=0;
				m.bmsg("   You won");
				fleet0 = getNumShips(atstarsystem,0);
		}
	}
  if (fleet0 >0)
	 return Attackstarsystem(0,fleet0,atstarsystem);
  else
	 return Attackstarsystem(9,fleet9,atstarsystem);
}

// handles all fights and reinforcements, between player 0 and enemy 9 
void galaxy::DoFights() {
	int i,f0,f9;
	f0=0;
	f9=0;
	for (i=0;i<maxstarsystems;i++) {
		f0= getNumShips(i,0);
		f9= getNumShips(i,9);
		if (f0 >0 || f9>0)
			fight(f0,f9,i);
	}
}

// index of nearest galaxy not owned by player 9
int galaxy::findNeareststarsystem(int atstarsystem) {
	int dist,sdist,ni,sx,sy,nsx,nsy,i;
	sx=starsystems[atstarsystem].x;	
	sy=starsystems[atstarsystem].y;
	sdist = 99;
	ni=-1;
	for (i=0;i< maxstarsystems;i++) {
		if (starsystems[i].owner != 9) 
		{
			nsx= starsystems[i].x;			
			nsy= starsystems[i].y;
			dist = distance(sx-nsx,sy-nsy);
			if (dist < sdist) {
				sdist = dist;
				ni = i;
			}
		}
	}
	return ni;
}


/* Every time this is called, it checks if one second passed and if not 
it returns. It's called from the keyboard input loop when no key is pressed. 
The message display is 15 lines of messages that scroll up.
Messages are written into a messages vector  using messages.size() as the index 
into the 100 lines to write the message. Then msghead follows msgtail and is where 
messages are displayed from. The tricky bit is when less than 15 lines of messages, 
write spaces (only happens at start).
Also, when more than 15 messages start removing the first one.
*/

bool messageHandler::CheckMessages() {
	int i,nummsgs,numbl,j,line;
	if (!onesecpassed())
		return false;
    line = msgline+1;
	nummsgs=messages.size();
	if (nummsgs==0)
	  return false; // no messages 
	if (nummsgs < 15) {
		numbl = 15-nummsgs;
		for(i=0;i<numbl;i++){
			crt.gotoxy(0,line+i);
			printf("%66s",spaces.c_str());
		}
		j=0;
		for (i=numbl;i<15;i++)
		{			
			crt.gotoxy(0,line+i);
			printf("%66s",spaces.c_str());			
			crt.gotoxy(0,line+i);
			printf("%s",messages[j++].c_str());
		}

	}
	else
	{		
		for (i=0;i<16;i++)
		{						
			crt.gotoxy(0,line+i);
			printf("%66s",spaces.c_str());	
			crt.gotoxy(0,line+i);
			printf("%s",messages[i++].c_str());
		}
		messages[i].erase();
	}
	return true; // Messages scrolled  
}

int main(int argc, char * argv[]) {
	game g;

	while (!g.GameOver) {
		g.DoTurn();

	}
	// Give game a chance to display all messages 
	while (g.ScrollMessages()) {
		waitms(500);
	}
	g.showmsg("Game Over - Closing down...");
	waitms(3*1000);
	return 0;
}