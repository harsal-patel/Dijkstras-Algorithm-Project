/*
* Created by Harsal Patel
* 
* Console text color changes and the ClearScreen() function work on Windows OS only. Please replace the ClearScreen() calls in main
* with system("cls") and comment out or delete the lines that change text color if you are running this on another OS.
*/

#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <conio.h> // for getch
#include <fstream>
#include <math.h>
#include <limits.h>
#include <ctype.h>
#include <Windows.h> // used to change console text color

using namespace std;

class Tile { // term used for each vertex in the map
public:
	char symbol; // used to display in map
	int vertex;
	double weight;
	Tile* next; // used for adjacency list
	int pi; // predecessor used for Dijkstra's
	Tile() {
		vertex = 0;
		weight = 0;
		next = nullptr;
		symbol = '!';
		pi = -1;
	}
	Tile(int vertex, double weight, char symbol) {
		this->vertex = vertex;
		this->weight = weight;
		next = nullptr;
		this->symbol = symbol;
		pi = -1;
	}
	~Tile() {}
};

struct Characters {
	int vertex;
	char tile;
	bool seesUser;
	int counter;
};

class Map {
private:
	int mapSize;
	int numVertices;
	Tile* map; // array of tiles representing a map
	Tile** list; // adjacency list, array of arrays
	Tile** current; // used in destructor
	Characters user; // info of player
	Characters* enemies; // array of enemies
	int numEnemies;
	int enemiesSize;
	int* hiddenTiles;
	int numHiddenTiles;
	int hiddenSize;
	bool canMove(int vertex, int direction);
	bool adjacentHidden(int vertex, int direction);
	bool adjacentPlayer(int vertex);
	void setVisibility();
	int minDistance(int* dist, bool* visited);
	int Dijkstra(int source, int target);
public:
	Map() {
		mapSize = 0;
		numVertices = 0;
		current = nullptr;
		list = nullptr;
		map = nullptr;
		enemies = nullptr;
		numEnemies = 0;
		enemiesSize = 0;
		hiddenTiles = nullptr;
		hiddenSize = 0;
		numHiddenTiles = 0;
	}
	Map(int n) {
		mapSize = n;
		numVertices = 0;
		list = new Tile * [mapSize];
		current = new Tile * [mapSize];
		map = new Tile[mapSize];
		for (int u = 0; u < n; u++) {
			list[u] = nullptr;
			current[u] = nullptr;
		}
		enemiesSize = 6;
		numEnemies = 0;
		enemies = new Characters[enemiesSize];
		numHiddenTiles = 0;
		hiddenSize = 8;
		hiddenTiles = new int[hiddenSize];
	}
	~Map() {
		for (int i = 0; i < mapSize; i++) {
			Tile* current = list[i];
			while (current) {
				Tile* next = current->next;
				delete current;
				current = next;
			}
		}
		delete[] list;
		delete[] current;
		delete[] map;
		delete[] enemies;
	}
	void expand();
	void expandEnemies();
	void expandHidden();
	bool hasEdge(int u, int v);
	double getWeight(int u, int v);
	void setEdge(int u, int v, double w, char s);
	void printList();
	void mapFromFile(string filename);
	void mapToGraph();
	void printMap();
	void moveEnemies();
	void move(char direction);
	void reset();
};

void Map::expand() { // map is of dynamic size
	mapSize *= 2;
	Tile* temp = new Tile[mapSize];
	for (int i = 0; i < numVertices; i++) {
		temp[i] = map[i];
	}
	delete[] map;
	map = temp;
	delete[] temp;
}

void Map::expandEnemies() { // enemy array is dynamic
	enemiesSize *= 2;
	Characters* temp = new Characters[enemiesSize];
	for (int i = 0; i < numEnemies; i++) {
		temp[i] = enemies[i];
	}
	delete[] enemies;
	enemies = temp;
	delete[] temp;
}

void Map::expandHidden() { // hidden tile array is dynamic
	hiddenSize *= 2;
	int* temp = new int[hiddenSize];
	for (int i = 0; i < numHiddenTiles; i++) {
		temp[i] = hiddenTiles[i];
	}
	delete[] hiddenTiles;
	hiddenTiles = temp;
	delete[] temp;
}

bool Map::hasEdge(int u, int v) { // checks adjacency list to see if there is an edge between vertex u and vertex v
	if (u < numVertices) {
		Tile* cursor = list[u];
		while (cursor) {
			if (cursor->vertex == v) { return true; }
			else { cursor = cursor->next; }
		}
		return false;
	}
	else { return false; }
}

double Map::getWeight(int u, int v) { // gets edge weight between two vertices
	if (u < numVertices) {
		Tile* cursor = list[u];
		while (cursor) {
			if (cursor->vertex == v) { return cursor->weight; }
			else { cursor = cursor->next; }
		}
		return 0;
	}
	else { return 0; }
}

void Map::setEdge(int u, int v, double w, char s) { // creates a directed edge between two vertices if an edge between them doesn't already exist.
	if (!hasEdge(u, v)) {
		Tile* temp = new Tile(v, w, s);
		temp->next = list[u];
		list[u] = temp;
	}
}

void Map::printList() { // prints adjacency list
	for (int u = 0; u < numVertices; u++) {
		cout << "| " << u << ":";
		Tile* cursor = list[u];
		while (cursor) {
			cout << " " << cursor->vertex << "(" << cursor->weight << ")";
			cursor = cursor->next;
		}
		cout << "\n";
	}
}

void Map::mapFromFile(string filename) { // fills map array with map from a file
	ifstream inFS(filename);
	char temp;
	while (inFS >> temp) {
		if (numVertices == mapSize) { expand(); } // expands map size in case it does not have room for an element
		double tempWeight;
		if (temp == 'X') { tempWeight = 0; }
		else if (temp == '-') { tempWeight = 2; }
		else { tempWeight = 1; }
		Tile tempTile(numVertices, tempWeight, (temp == '_') ? ' ' : temp); // '_' in text file represents plain terrain in the map
		tempTile.pi = -1; // sets predecessor to -1
		map[numVertices] = tempTile;
		if (tempTile.symbol == 'H') { // loads the array containing vertices of all hidden tiles
			if (numHiddenTiles == hiddenSize) { expandHidden(); }
			hiddenTiles[numHiddenTiles] = numVertices;
			numHiddenTiles++;
		}
		if (tempTile.symbol == 'O') { // sets values for user
			user.vertex = numVertices;
			user.tile = ' ';
			user.seesUser = false;
			user.counter = 0;
		}
		if (tempTile.symbol == '#') { // adds each enemy to the enemy array
			if (numEnemies == enemiesSize) { expandEnemies(); }
			Characters temp;
			temp.vertex = numVertices;
			temp.tile = ' ';
			temp.seesUser = false;
			temp.counter = 0;
			enemies[numEnemies] = temp;
			numEnemies++;
		}
		numVertices++;
	}
	inFS.close();
}

void Map::mapToGraph() { // converts map array into a graph
	int rows = sqrt(numVertices);
	for (int u = 0; u < mapSize; u++) { // iterates through all tiles in map and checks if there is a tile you can move to from there
		int left = u - 1, right = u + 1, up = u - rows, down = u + rows;
		if (map[u].symbol != 'X') {
			if (left >= 0 && map[left].symbol != 'X') { // checks left tile
				setEdge(u, left, map[left].weight, map[left].symbol);
			}
			if (right < numVertices && map[right].symbol != 'X') { // checks right tile
				setEdge(u, right, map[right].weight, map[right].symbol);
			}
			if (up >= 0 && map[up].symbol != 'X') { // checks above tile
				setEdge(u, up, map[up].weight, map[up].symbol);
			}
			if (down < numVertices && map[down].symbol != 'X') { // checks below tile
				setEdge(u, down, map[down].weight, map[down].symbol);
			}
		}
	}
}

void Map::printMap() { // prints map
	int counter = 0;
	cout << "\t";
	for (int i = 0; i < numVertices; i++) {
		// Color changes are only available on Windows OS, please comment out or delete the lines that correspond to color changes if on another OS
		// Sets colors based on type of tile
		if (map[i].symbol == 'X') { SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 8); } // gray
		if (map[i].symbol == '#') { SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 4); } // red
		if (map[i].symbol == '-') { SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 2); } // green
		if (map[i].symbol == 'O') { SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 14); } // tan
		if (map[i].symbol == 'H') { SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 11); } // cyan
		cout << map[i].symbol << " ";
		counter++;
		if (counter == sqrt(numVertices)) { // displays map array as 2d array by inserting a newline after a row is completed
			cout << "\n\t";
			counter = 0;
		}
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15); // resets text color to white
	}
}

int Map::minDistance(int* dist, bool* visited) { // used in Dijkstra's algorithm for selecting the next vertex
	int minIndex = -1;
	int min = INT_MAX;

	for (int i = 0; i < numVertices; i++) {
		if (visited[i] == false && dist[i] <= min) {
			min = dist[i];
			minIndex = i;
		}
	}
	return minIndex;
}
// modified Dijkstra's algorithm that finds the path from a source to a target
int Map::Dijkstra(int source, int target) {
	int* dist = new int[numVertices];
	bool* visited = new bool[numVertices];
	bool done = false;

	for (int i = 0; i < numVertices; i++) {
		dist[i] = INT_MAX;
		visited[i] = false;
	}

	dist[source] = 0;

	for (int count = 0; count < numVertices - 1; count++) {
		int u = minDistance(dist, visited);
		visited[u] = true;
		if (!done) {
			for (int v = 0; v < numVertices; v++) {
				if (!visited[v] && hasEdge(u, v) && dist[u] != INT_MAX && dist[u] + getWeight(u, v) < dist[v] && map[v].symbol != 'H') {
					dist[v] = dist[u] + getWeight(u, v);
					map[v].pi = u;
				}
			}
		}
		if (u == target) { done = true; }
	}
	int next = map[target].pi;
	while (map[next].pi != source) { next = map[next].pi; } // backtracks through each predecessor until the next step is calculated
	return next;
}
// function to check if character can move in a certain direction
bool Map::canMove(int vertex, int direction) {
	int rows = sqrt(numVertices);

	if (direction == 0) { // up
		int up = vertex - rows;
		if (up >= 0 && (map[up].symbol == ' ' || map[up].symbol == '-')) { return true; }
	}
	else if (direction == 1) { // down
		int down = vertex + rows;
		if (down < numVertices && (map[down].symbol == ' ' || map[down].symbol == '-')) { return true; }
	}
	else if (direction == 2) { // left
		int left = vertex - 1;
		if (left >= 0 && (map[left].symbol == ' ' || map[left].symbol == '-')) { return true; }
	}
	else { // right
		int right = vertex + 1;
		if (right < numVertices && (map[right].symbol == ' ' || map[right].symbol == '-')) { return true; }
	}

	return false;
}
// used for user movement since user can move to hidden tiles as well as where enemies can
bool Map::adjacentHidden(int vertex, int direction) {
	int rows = sqrt(numVertices);

	if (direction == 0) { // up
		int up = vertex - rows;
		if (up >= 0 && (map[up].symbol == 'H')) { return true; }
	}
	else if (direction == 1) { // down
		int down = vertex + rows;
		if (down < numVertices && (map[down].symbol == 'H')) { return true; }
	}
	else if (direction == 2) { // left
		int left = vertex - 1;
		if (left >= 0 && (map[left].symbol == 'H')) { return true; }
	}
	else { // right
		int right = vertex + 1;
		if (right < numVertices && (map[right].symbol == 'H')) { return true; }
	}

	return false;
}
// checks if enemy is adjacent to user
bool Map::adjacentPlayer(int vertex) {
	int rows = sqrt(numVertices);
	if (user.vertex == vertex - rows || user.vertex == vertex + rows || user.vertex == vertex - 1 || user.vertex == vertex + 1) { return true; }
	return false;
}
// This function checks 8 tiles left, right, up and down to see if the player is visible to the enemy
void Map::setVisibility() {
	for (int i = 0; i < numEnemies; i++) {
		int counter = 1;
		int up = enemies[i].vertex, down = enemies[i].vertex, left = enemies[i].vertex, right = enemies[i].vertex, rows = sqrt(numVertices);
		while (counter <= 8) {
			if (map[up].symbol != 'X' && enemies[i].vertex - (rows * counter) >= 0) { up = enemies[i].vertex - (rows * counter); }
			if (map[down].symbol != 'X' && enemies[i].vertex + (rows * counter) < numVertices) { down = enemies[i].vertex + (rows * counter); }
			if (map[left].symbol != 'X' && enemies[i].vertex - counter >= 0) { left = enemies[i].vertex - counter; }
			if (map[right].symbol != 'X' && enemies[i].vertex + counter < numVertices) { right = enemies[i].vertex + counter; }
			if (map[up].symbol == 'O' && user.tile != 'H') { enemies[i].seesUser = true; }
			if (map[down].symbol == 'O' && user.tile != 'H') { enemies[i].seesUser = true; }
			if (map[left].symbol == 'O' && user.tile != 'H') { enemies[i].seesUser = true; }
			if (map[right].symbol == 'O' && user.tile != 'H') { enemies[i].seesUser = true; }

			counter++;
		}
	}
}
// function used for enemy movement
void Map::moveEnemies() {
	for (int i = 0; i < numEnemies; i++) {
		if (!enemies[i].seesUser) { // move randomly if user is not visible
			int temp = rand() % 4;
			// makes enemy wait a turn if it is unable to move to any spot around it
			if (!canMove(enemies[i].vertex, 0) && !canMove(enemies[i].vertex, 1) && !canMove(enemies[i].vertex, 2) && !canMove(enemies[i].vertex, 3)) { temp = -1; }
			else {
				while (!canMove(enemies[i].vertex, temp)) { temp = rand() % 4; } // randomly pick a number from 0-3 until that number is a viable direction
			}
			if (temp == 0) { // up
				if (enemies[i].tile == '-' && enemies[i].counter == 0) { enemies[i].counter++; } // grass takes additional step to move through
				else {
					map[enemies[i].vertex].symbol = enemies[i].tile;
					enemies[i].vertex -= sqrt(numVertices);
					enemies[i].tile = map[enemies[i].vertex].symbol;
					map[enemies[i].vertex].symbol = '#';
					enemies[i].counter = 0;
				}
			}
			else if (temp == 1) { // down
				if (enemies[i].tile == '-' && enemies[i].counter == 0) { enemies[i].counter++; }
				else {
					map[enemies[i].vertex].symbol = enemies[i].tile;
					enemies[i].vertex += sqrt(numVertices);
					enemies[i].tile = map[enemies[i].vertex].symbol;
					map[enemies[i].vertex].symbol = '#';
					enemies[i].counter = 0;
				}
			}
			else if (temp == 2) { // left
				if (enemies[i].tile == '-' && enemies[i].counter == 0) { enemies[i].counter++; }
				else {
					map[enemies[i].vertex].symbol = enemies[i].tile;
					enemies[i].vertex -= 1;
					enemies[i].tile = map[enemies[i].vertex].symbol;
					map[enemies[i].vertex].symbol = '#';
					enemies[i].counter = 0;
				}
			}
			else if (temp == 3) { // right
				if (enemies[i].tile == '-' && enemies[i].counter == 0) { enemies[i].counter++; }
				else {
					map[enemies[i].vertex].symbol = enemies[i].tile;
					enemies[i].vertex += 1;
					enemies[i].tile = map[enemies[i].vertex].symbol;
					map[enemies[i].vertex].symbol = '#';
					enemies[i].counter = 0;
				}
			}
		}
		else { // if enemy can see the user, either move using Dijkstra or move onto user space
			if (enemies[i].tile == '-' && enemies[i].counter == 0) { enemies[i].counter++; }
			else if (!adjacentPlayer(enemies[i].vertex)) {
				int next = Dijkstra(enemies[i].vertex, user.vertex);
				if (map[next].symbol != '#') {
					map[enemies[i].vertex].symbol = enemies[i].tile;
					enemies[i].vertex = next;
					enemies[i].tile = map[enemies[i].vertex].symbol;
					map[next].symbol = '#';
				}
				enemies[i].counter = 0;
			}
			else {
				map[enemies[i].vertex].symbol = enemies[i].tile;
				enemies[i].tile = user.tile;
				enemies[i].vertex = user.vertex;
				map[enemies[i].vertex].symbol = '#';
				int random = rand() % numHiddenTiles; // moves user to random hidden tile if caught
				user.tile = 'H';
				user.vertex = hiddenTiles[random];
				map[hiddenTiles[random]].symbol = 'O';
				enemies[i].counter = 0;
				user.counter = 0;
				cout << "You've been caught! Respawning at random hidden tile...";
				for (int i = 0; i < numEnemies; i++) { // when caught, enemies no longer see user
					enemies[i].seesUser = false;
				}
				_getch();
				system("cls");
				printMap();
			}
		}
	}
	setVisibility();
}

void Map::move(char direction) { // main movement function since user moves before enemies
	int n;
	direction = tolower(direction);
	if (direction == 'w' || direction == 'W') { n = 0; } // checks for valid direction
	else if (direction == 's' || direction == 'S') { n = 1; }
	else if (direction == 'a' || direction == 'A') { n = 2; }
	else if (direction == 'd' || direction == 'D') { n = 3; }
	else { n = -1; }
	if (n >= 0) {
		if (canMove(user.vertex, n) || adjacentHidden(user.vertex, n)) { // player can move where enemies can but also to hidden tiles
			if (n == 0) { // up
				if (user.tile == '-' && user.counter == 0) { user.counter++; } // grass takes additional step to move through
				else {
					map[user.vertex].symbol = user.tile;
					user.vertex -= sqrt(numVertices);
					user.tile = map[user.vertex].symbol;
					map[user.vertex].symbol = 'O';
					user.counter = 0;
				}
			}
			else if (n == 1) { // down
				if (user.tile == '-' && user.counter == 0) { user.counter++; }
				else {
					map[user.vertex].symbol = user.tile;
					user.vertex += sqrt(numVertices);
					user.tile = map[user.vertex].symbol;
					map[user.vertex].symbol = 'O';
					user.counter = 0;
				}
			}
			else if (n == 2) { // left
				if (user.tile == '-' && user.counter == 0) { user.counter++; }
				else {
					map[user.vertex].symbol = user.tile;
					user.vertex -= 1;
					user.tile = map[user.vertex].symbol;
					map[user.vertex].symbol = 'O';
					user.counter = 0;
				}
			}
			else { // right
				if (user.tile == '-' && user.counter == 0) { user.counter++; }
				else {
					map[user.vertex].symbol = user.tile;
					user.vertex += 1;
					user.tile = map[user.vertex].symbol;
					map[user.vertex].symbol = 'O';
					user.counter = 0;
				}
			}
			if (user.tile == 'H') { // enemies lose sight of user if user is on hidden tile
				for (int i = 0; i < numEnemies; i++) {
					enemies[i].seesUser = false;
				}
			}
			moveEnemies(); // enemies move after user
		}
	}
	setVisibility(); // after all movement, check if enemies can see user
}
// acts as a destructor then constructor to reset instance of Map to allow for current map to be overwritten
void Map::reset() {
	for (int i = 0; i < mapSize; i++) {
		Tile* current = list[i];
		while (current) {
			Tile* next = current->next;
			delete current;
			current = next;
		}
	}
	delete[] list;
	delete[] current;
	delete[] map;
	delete[] enemies;

	numVertices = 0;
	list = new Tile * [mapSize];
	current = new Tile * [mapSize];
	map = new Tile[mapSize];
	for (int u = 0; u < mapSize; u++) {
		list[u] = nullptr;
		current[u] = nullptr;
	}
	numEnemies = 0;
	enemies = new Characters[enemiesSize];
	numHiddenTiles = 0;
	hiddenTiles = new int[hiddenSize];
}
// function to print main menu
void PrintMenu() {
	cout << "\n\n";
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 9);
	cout << "       _____  _ _ _        _             _        _____ _                _            _     _____      _   _     " << endl;
	cout << "      |  __ \\(_|_) |      | |           ( )      / ____| |              | |          | |   |  __ \\    | | | |    " << endl;
	cout << "      | |  | |_ _| | _____| |_ _ __ __ _|/ ___  | (___ | |__   ___  _ __| |_ ___  ___| |_  | |__) |_ _| |_| |__  " << endl;
	cout << "      | |  | | | | |/ / __| __| '__/ _` | / __|  \\___ \\| '_ \\ / _ \\| '__| __/ _ \\/ __| __| |  ___/ _` | __| '_ \\ " << endl;
	cout << "      | |__| | | |   <\\__ \\ |_| | | (_| | \\__ \\  ____) | | | | (_) | |  | ||  __/\\__ \\ |_  | |  | (_| | |_| | | |" << endl;
	cout << "      |_____/|_| |_|\\_\\___/\\__|_|  \\__,_| |___/ |_____/|_| |_|\\___/|_|   \\__\\___||___/\\__| |_|   \\__,_|\\__|_| |_|" << endl;
	cout << "              _/ |                                                                                               " << endl;
	cout << "             |__/" << endl;
	cout << "  __-__-__-__-__-__-__-__-__-__-__-__-__-__-__-__-__-__-__-__-__-__-__-__-__-__-__-__-__-__-__-__-__-__-__-__-__-__-__";
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
	cout << "\n\n\n\n\t\t\t\t\t\t     MAIN MENU\n\n";
	cout << "\t\t\t\t\t\tw. Start/continue game\n";
	cout << "\t\t\t\t\t\ta. View example map\n";
	cout << "\t\t\t\t\t\ts. Controls and info\n";
	cout << "\t\t\t\t\t\td. Reset\n";
	cout << "\t\t\t\t\t\tq. Quit game\n";
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
}

char GetInput() { // gets user input using getch, accounts for capitalization
	cout << "\n\n\t\t\t\t\t\tEnter choice:";
	char temp = _getch();
	temp = tolower(temp);
	return temp;
}
// clear screen function for use during game (to prevent flickering)
void ClearScreen() {
	COORD cursorPosition;
	cursorPosition.X = 0;
	cursorPosition.Y = 0;
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), cursorPosition);
}

int main() {
	srand(time(0));

	char input;
	bool endGame = false;

	Map testMap(64);
	testMap.mapFromFile("map1.txt");
	testMap.mapToGraph();

	Map map(784);
	int mapNumber = rand() % 5;
	int currentMap = mapNumber;
	if (mapNumber == 0) { map.mapFromFile("map2.txt"); }
	else if (mapNumber == 1) { map.mapFromFile("map3.txt"); }
	else if (mapNumber == 2) { map.mapFromFile("map4.txt"); }
	else if (mapNumber == 3) { map.mapFromFile("map5.txt"); }
	else if (mapNumber == 4) { map.mapFromFile("map6.txt"); }
	map.mapToGraph();

	PrintMenu();
	input = GetInput();

	while (!endGame) {
		switch (input) {
		case 'w': // starts/continues game
			system("cls");
			map.printMap();
			input = '-';
			while (tolower(input) != 'q') { // loops while user does not press 'q'
				input = _getch();
				if (input == ' ') {
					map.moveEnemies();
					ClearScreen(); // use system("cls") instead if not on Windows OS
					map.printMap();
				}
				else {
					map.move(input);
					ClearScreen(); // use system("cls") instead if not on Windows OS
					map.printMap();
				}
			}
			input = ' ';
			break;
		case 'a': // show example map and adjacency list
			system("cls");
			cout << "\n\tExample map:\n\n";
			testMap.printMap();
			cout << "\nPress any key to print adjacency list...\n";
			input = _getch();
			cout << "\n| Adjacency List:\n\n";
			testMap.printList();
			cout << "\n\nPress any key to return to main menu...";
			input = _getch();
			system("cls");
			PrintMenu();
			input = ' ';
			break;
		case 's': // prints controls and instructions
			system("cls");
			cout << "\n| Controls and Info:\n\n";
			cout << "| The object of the game is to not be caught by an enemy. The enemies will move around the map randomly until you are\n";
			cout << "| in sight. They will then move towards you until they reach you or you get to a hidden tile. If they reach you, you\n";
			cout << "| will respawn at a random hidden tile within the map.\n\n";
			cout << "| Move with W, A, S, and D (up, left, down, and right respectively)\n";
			cout << "| Press Space if you would like to skip a turn (the enemies will still move)\n";
			cout << "| Press Q at any point during the game to quit\n\n";
			cout << "| Map tiles:\n";
			cout << "| Blank tiles are plain ground and require 1 move to move across\n";
			cout << "|";
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 2);
			cout << " -";
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
			cout << " tiles are grassy ground and require 2 moves to move across\n";
			cout << "|";
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 11);
			cout << " H";
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
			cout << " tiles are hidden spots where the enemies cannot see you\n";
			cout << "|";
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 8);
			cout << " X";
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
			cout << " tiles are walls that neither you nor the enemies can walk through\n";
			cout << "|";
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 4);
			cout << " #";
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
			cout << " characters are the enemies, avoid them as you travel around the map\n";
			cout << "|";
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 9);
			cout << " O";
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
			cout << " is your character\n\n";
			cout << "Press any key to return to the main menu...";
			input = _getch();
			system("cls");
			PrintMenu();
			input = ' ';
			break;
		case 'd': // changes map to one of the other possibilites
			map.reset();
			while (mapNumber == currentMap) { mapNumber = rand() % 5; }
			currentMap = mapNumber;
			if (mapNumber == 0) { map.mapFromFile("map2.txt"); }
			else if (mapNumber == 1) { map.mapFromFile("map3.txt"); }
			else if (mapNumber == 2) { map.mapFromFile("map4.txt"); }
			else if (mapNumber == 3) { map.mapFromFile("map5.txt"); }
			else if (mapNumber == 4) { map.mapFromFile("map6.txt"); }
			map.mapToGraph();
			input = ' ';
			break;
		case 'q': // sets bool to true to exit program loop
			endGame = true;
			break;
		default: // invalid inputs do not do anything, prompts user to input again
			system("cls");
			PrintMenu();
			input = GetInput();
		}
	}

	return 0;
}