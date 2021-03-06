#include <algorithm>
#include <fstream>
#include <iostream>
#include <math.h>
#include <queue>
#include <sstream>
#include <stack>
#include <string>
#include <stdlib.h>
#include <vector>

using namespace std;

#define bend_cost 10
#define via_cost 20

#define gCell_pred_bits 0xFF000000
#define gCell_obstacle_bit 0x00800000
#define gCell_reached_bit 0x004000000
#define gCell_layer_bits 0x003C0000
#define gCell_cost_bits 0x0003FFFF
#define wfCell_x_bits 0xFFC0000000000000
#define wfCell_y_bits 0x003FF00000000000
#define wfCell_layer_bits 0x00000F0000000000
#define wfCell_pred_bits 0x000000FF00000000
#define wfCell_cost_bits 0x00000000FFFFFFFF

/*
net1 (1, 42, 67) (1, 60, 14)
net2 (1, 10, 20) (1, 30, 50)
net3 (1, 104, 201) (1, 303, 502)
*/
/*
net1 (1, 10, 10) (1, 15, 15)
net2 (1, 14, 15) (1, 20, 20)
net3 (1, 12, 14) (1, 17, 18)
*/
// Pins data
struct pins {
	int layer, x_coordinate, y_coordinate;
	int distance_to_center;
};

int layer_1[1000][1000];

class Comparator
{
public:
	long long operator() (long long& w1, long long& w2)
	{
		long long cost1 = w1 & (wfCell_cost_bits);
		long long cost2 = w2 & (wfCell_cost_bits);
		return cost1 > cost2;
	}
};

int calc_distance_to_center(pins pin)
{
	return sqrt(pow(abs(pin.x_coordinate - 500), 2) + pow(abs(pin.y_coordinate - 500), 2));
}

void read_line(string input_line, vector<pins>& pins_vector)
{
	pins pin;
	size_t comma_substr, temp_pos, temp;
	do {
		temp_pos = input_line.find("(") + 1;
		comma_substr = input_line.find(",");
		pin.layer = atoi(input_line.substr(temp_pos, comma_substr).c_str());
		input_line = input_line.substr(comma_substr + 2);
		comma_substr = input_line.find(",");
		pin.x_coordinate = atoi(input_line.substr(0, comma_substr).c_str());
		input_line = input_line.substr(comma_substr + 2);
		comma_substr = input_line.find(")");
		pin.y_coordinate = atoi(input_line.substr(0, comma_substr).c_str());
		pin.distance_to_center = calc_distance_to_center(pin);
		pins_vector.push_back(pin);
	} while (input_line.find("(") != -1);
}

void predec_location(int& l, int& x, int& y, const char& direction, bool& source)
{
	source = false;
	switch (direction)
	{
	case 'N':
		y--;
		break;
	case 'S':
		y++;
		break;
	case 'W':
		x--;
		break;
	case 'E':
		x++;
		break;
	case 'U':
		l--;
		break;
	case 'D':
		l++;
		break;
	default:
		source = true;
		break;
	}
}

long long convert_gridCell_to_wfCell(const int& gCell, const int& x, const int& y)
{
	/*
	pins vector --> x , y, layer, distance
	 wavefront --> x, y, layer, pathcost, predecessor
	 source --> indexed by x, y, layer (from pins_vector) (stores ..)
	 x = 10 bits , y = 10 bits ; layer = 4 bits , predecessor = 8 bits , pathcost = 19 bits
	#define gCell_pred_bits 0xFF000000
	#define gCell_obstacle_bit 0x00800000
	#define gCell_reached_bit 0x004000000
	#define gCell_layer_bits 0x003C0000
	#define gCell_cost_bits 0x0003FFFF
	#define wfCell_x_bits 0xFFC0000000000000
	#define wfCell_y_bits 0x003FF00000000000
	#define wfCell_layer_bits 0x00000F0000000000
	#define wfCell_pred_bits 0x000000FF00000000
	#define wfCell_cost_bits 0x00000000FFFFFFFF
	gCell -> pred = 8 bits, obstacle = 1 bit, reached = 1 bit, layer = 4 bit, cost = 18 bits
	wfCell -> x = 10 bits, y = 10 bits, layer = 4 bits, predecessor = 8 bits, pathcost = 32 bits

	gCell
	100-0101-0000-0100-0000-0000-0000-0010
	wfCell
	10-0100-0001-0100-0001-0100-0000-0000-0000-0000-0000-0000-0000-0000-0010

	*/
	long long wfCell = 0;
	//wfCell = (((long long(x) << 54) & wfCell_x_bits) | ((long long(y) << 44) & wfCell_y_bits) |
	//	(((long long(gCell) & gCell_layer_bits) << 22) & wfCell_layer_bits) |
	//	(((long long(gCell) & gCell_pred_bits) << 8) & wfCell_pred_bits) |
	//	((long long(gCell) & gCell_cost_bits) & wfCell_cost_bits));
	wfCell = (((long long(x) << 54)) | ((long long(y) << 44)) | ((long long(gCell) & gCell_layer_bits) << 22)
		| ((long long(gCell) & gCell_pred_bits) << 8) | (long long(gCell) & gCell_cost_bits));
//		((long long(gCell) & wfCell_layer_bits)));
		//((long long(gCell) & wfCell_pred_bits) << 8) |
		//((long long(gCell)) & wfCell_cost_bits));
	return wfCell;
}

bool route(vector<pins>& pins_vector, string& path_str)
{
	priority_queue<long long, vector<long long>, Comparator> wavefront;
	stack<string> path;
	long long source = ((wfCell_x_bits) & (long long(pins_vector[0].x_coordinate) << 54));
	source = source | ((long long(pins_vector[0].y_coordinate) << 44) & (wfCell_y_bits));
	source = source | ((long long(pins_vector[0].layer) << 40) & (wfCell_layer_bits));
	source = source | 1; //cost is to the right most
	layer_1[pins_vector[0].x_coordinate][pins_vector[0].y_coordinate] = layer_1[pins_vector[0].x_coordinate][pins_vector[0].y_coordinate] | (gCell_reached_bit);
	long long target = ((wfCell_x_bits) & (long long(pins_vector[1].x_coordinate) << 54));
	target = target | ((long long(pins_vector[1].y_coordinate) << 44) & (wfCell_y_bits));
	target = target | ((long long(pins_vector[1].layer) << 40) & (wfCell_layer_bits));
	target = target | 1; //cost is to the rightmost
	wavefront.push(source);
	long long wf_top;
	//int source_cost;
	while ((layer_1[pins_vector[1].x_coordinate][pins_vector[1].y_coordinate] & gCell_reached_bit) != 1)
	{
		if (wavefront.empty()) // path not found
			return false;
		else
		{
			bool s;
			wf_top = wavefront.top();
			int x = (wf_top & (wfCell_x_bits)) >> 54;
			int y = (wf_top & (wfCell_y_bits)) >> 44;
			int l = (wf_top & (wfCell_layer_bits)) >> 40;
			char dir = (wf_top & wfCell_pred_bits) >> 32;
			if ((wf_top & (wfCell_x_bits | wfCell_y_bits | wfCell_layer_bits)) == (target & (wfCell_x_bits | wfCell_y_bits | wfCell_layer_bits)))
			{	// && (l != ((source & wfCell_layer_bits) >> 40)
				//while ((x != ((source & wfCell_x_bits) >> 54)) && (y != ((source & wfCell_y_bits) >> 44)))
				while (source != layer_1[x][y])
				{
					string substr = " (" + to_string(l) + ", " + to_string(x) + ", " + to_string(y) + ") ";
					path.push(substr);
					layer_1[x][y] = layer_1[x][y] | (gCell_obstacle_bit);
					char dir_old = (layer_1[x][y] & gCell_pred_bits) >> 24;
					predec_location(l, x, y, dir_old, s); // must indicate obstacle
					dir = (layer_1[x][y] & gCell_pred_bits) >> 24;
					if (s)
						break;
				}
				while (!path.empty())
				{
					path_str += path.top();
					path.pop();
				}
				return true;
			}
			if (x > 0 && y > 0 && !(layer_1[x - 1][y] & gCell_reached_bit) && !(layer_1[x - 1][y] & gCell_obstacle_bit))
			{
				layer_1[x - 1][y] = layer_1[x - 1][y] | gCell_reached_bit;
				// pathcost(N) = pathcost(C) + cellcost(N) --> support of A* heuristic
				layer_1[x - 1][y] += (layer_1[x][y] & gCell_cost_bits);
				layer_1[x - 1][y] = layer_1[x - 1][y] | (long long('E') << 24);
				wavefront.push(convert_gridCell_to_wfCell(layer_1[x - 1][y], x - 1, y));
			}
			if (x > 0 && y > 0 && !(layer_1[x][y - 1] & gCell_reached_bit) && !(layer_1[x][y - 1] & gCell_obstacle_bit))
			{
				layer_1[x][y - 1] = layer_1[x][y - 1] | gCell_reached_bit;
				layer_1[x][y - 1] += (layer_1[x][y] & gCell_cost_bits) + bend_cost;
				layer_1[x][y - 1] = layer_1[x][y - 1] | (long long('S') << 24);
				wavefront.push(convert_gridCell_to_wfCell(layer_1[x][y - 1], x, y - 1));
			}
			if (x > 0 && y > 0 && !(layer_1[x + 1][y] & gCell_reached_bit) && !(layer_1[x + 1][y] & gCell_obstacle_bit))
			{
				layer_1[x + 1][y] = layer_1[x + 1][y] | gCell_reached_bit;
				layer_1[x + 1][y] += (layer_1[x][y] & gCell_cost_bits);
				layer_1[x + 1][y] = layer_1[x + 1][y] | (long long('W') << 24);
				wavefront.push(convert_gridCell_to_wfCell(layer_1[x + 1][y], x + 1, y));
			}
			if (x > 0 && y > 0 && !(layer_1[x][y + 1] & gCell_reached_bit) && !(layer_1[x][y + 1] & gCell_obstacle_bit))
			{
				layer_1[x][y + 1] = layer_1[x][y + 1] | gCell_reached_bit;
				layer_1[x][y + 1] += (layer_1[x][y] & gCell_cost_bits) + bend_cost;
				layer_1[x][y + 1] = layer_1[x][y + 1] | (long long('N') << 24);
				wavefront.push(convert_gridCell_to_wfCell(layer_1[x][y + 1], x, y + 1));
			}
			wavefront.pop();
		}
	}
}

void initialize_layer()
{
	//initializing cells
	int x_axis, y_axis;
	// cin >> x_axis >> y_axis;
	// 
	int layer = 1;
	for (int i = 0; i < 1000; i++)
		for (int j = 0; j < 1000; j++)
		{
			layer_1[i][j] = (layer_1[i][j] & gCell_obstacle_bit);
			layer_1[i][j] |= (layer << 18);
			layer_1[i][j] |= 1;
		}
}

void find_neareast_source()
{

}

int main()
{
	int net_no = 1;
	ifstream input_file;
	ofstream output_file;
	vector <pins> pins_vector;
	string input_line;
	input_file.open("input_file.txt");
	output_file.open("output_file.txt", ios::out | ios::trunc);
	if (!input_file.is_open())
	{
		cout << "Unable to open input file\n";
		return 0;
	}
	while (!input_file.eof())
	{
		initialize_layer();
		getline(input_file, input_line);
		// Reading input file
		read_line(input_line, pins_vector);
		// Sorting nearest to boundary for starting point selection technique
		sort(pins_vector.begin(), pins_vector.end(), [](const pins& lhs, const pins& rhs) {
			return lhs.distance_to_center > rhs.distance_to_center;
			});
		//for (int i = 0; i < pins_vector.size(); i++)
			//cout << pins_vector[i].x_coordinate << " " << pins_vector[i].y_coordinate << " " << pins_vector[i].layer << " " << pins_vector[i].distance_to_center << "\n";
		//cout << "\n";
		// CORE CODE HERE ..................... (for each line, u have some pins)
		string path = "";
		if (route(pins_vector, path))
			cout << "net" << net_no++ << path << endl << endl;
		else
			cout << "Couldn't route!\n";
		pins_vector.clear();
	}
	input_file.close();
	output_file.close();
	return 0;
}
