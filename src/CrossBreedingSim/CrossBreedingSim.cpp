// CrossBreedingSim.cpp : このファイルには 'main' 関数が含まれています。プログラム実行の開始と終了がそこで行われます。
//

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <set>
#include <map>
#include "CrossBreedingSim.h"

using byte = unsigned char;

// 種,白,黄,赤,明,色
struct flower_t {
    bool is_seed = false;
    union {
        byte gene;
        struct {
            byte white  : 2;
            byte yellow : 2;
            byte red    : 2;
            byte brightness : 2;
        };
    };
    std::string color;
};

bool operator==(flower_t a, flower_t b) {
    return a.gene == b.gene;
}

bool operator<(flower_t a, flower_t b) {
    return a.gene < b.gene;
}

std::string to_string(flower_t f) {
    std::stringstream ss;
    
    f.is_seed ? ss << "(S) " : ss << "    ";
    ss << (f.white == 0 ? "00" : f.white == 1 ? "01" : "11");
    ss << "-";
    ss << (f.yellow == 0 ? "00" : f.yellow == 1 ? "01" : "11");
    ss << "-";
    ss << (f.red == 0 ? "00" : f.red == 1 ? "01" : "11");
    ss << "-";
    ss << (f.brightness == 0 ? "00" : f.brightness == 1 ? "01" : "11");
    ss << " ";
    ss << "(" << f.color << ")";
    ss << std::string(8 - f.color.length(), ' ');

    return ss.str();
}

// 花A * 花B = 花H (確率p) 
struct crossing_t {
    flower_t a;
    flower_t b;
    flower_t hybrid;
    double propability;
};

std::vector<byte> cross(byte a, byte b) {
    std::set<byte> pos_a; pos_a.insert(a); pos_a.insert(a == 0b01 ? 0b10 : a);
    std::set<byte> pos_b; pos_b.insert(b); pos_b.insert(b == 0b01 ? 0b10 : b);
    std::vector<byte> pos_ab;
    for (byte aa : pos_a) {
        byte xa = aa & 0b10, ya = aa & 0b01;
        for (byte bb : pos_b) {
            byte xb = bb & 0b10, yb = bb & 0b01;
            pos_ab.push_back(xa | yb);
            pos_ab.push_back(ya | xb);
        }
    }
    std::vector<byte> pos;
    for (byte xy : pos_ab) {
        pos.push_back(xy == 0b10 ? 0b01 : xy);
    }
    return pos;
}

std::vector<flower_t> cross(const flower_t &a, const flower_t &b, const std::map<byte, flower_t> &gene_map) {
    std::vector<byte> pos_w = cross(a.white, b.white);
    std::vector<byte> pos_y = cross(a.yellow, b.yellow);
    std::vector<byte> pos_r = cross(a.red, b.red);
    std::vector<byte> pos_b = cross(a.brightness, b.brightness);
    std::vector<flower_t> pos;
    for (byte w : pos_w) {
        for (byte y : pos_y) {
            for (byte r : pos_r) {
                for (byte b : pos_b) {
                    flower_t f;
                    f.white = w;
                    f.yellow = y;
                    f.red = r;
                    f.brightness = b;
                    auto itr = gene_map.find(f.gene);
                    if (itr != gene_map.end()) {
                        pos.push_back(itr->second);
                    }
                }
            }
        }
    }
    return pos;
}

void create_list(std::filesystem::path csv_path) {

    std::cout << "CSV: " <<  csv_path.filename() << std::endl;

    // read csv
    std::vector<flower_t> flowers;
    std::map<byte, flower_t> flowers_index;
    std::ifstream ifs(csv_path);
    std::string line;
    while (std::getline(ifs, line)) {
        std::stringstream ss(line);
        std::string cell;
        std::vector<std::string> cs;
        while (std::getline(ss, cell, ',')) {
            cs.push_back(cell);
        }
        flower_t flower;
        if (cs[0] != "" && cs[0] != "1") continue;
        if (cs[0] == "1") flower.is_seed = true;
        flower.yellow = (cs[1] == "00" ? 0b00 : cs[1] == "01" ? 0b01 : 0b11);
        flower.red = (cs[2] == "00" ? 0b00 : cs[2] == "01" ? 0b01 : 0b11);
        flower.white = (cs[3] == "00" ? 0b00 : cs[3] == "01" ? 0b01 : 0b11);
        flower.brightness = (cs[4] == "00" ? 0b00 : cs[4] == "01" ? 0b01 : 0b11);
        flower.color = cs[5];
        flowers.push_back(flower);
        flowers_index[flower.gene] = flower;
    }

    // cross breeding
    std::vector<crossing_t> crossing_list;
    std::set<flower_t> origin;
    std::set<flower_t> new_origin;
    for (auto f : flowers) {
        if (f.is_seed) {
            origin.insert(f);
            new_origin.insert(f);
        }
    }
    while (!new_origin.empty()) {
        std::vector<flower_t> crossed;
        for (auto fa : new_origin) {
            for (auto fb : origin) {
                std::vector<flower_t> possible = cross(fa, fb, flowers_index);
                double unit = 1.0 / possible.size();
                std::map<flower_t, double> prob;
                std::map<std::string, int> color_count;
                for (auto f : possible) {
                    prob[f] += unit;
                }
                for (auto itr : prob) {
                    color_count[itr.first.color]++;
                }
                for (auto itr : prob) {
                    auto f = itr.first;
                    if (f == fa || f == fb) continue;
                    if (color_count[f.color] == 1) {
                        crossing_t cr;
                        cr.a = fa;
                        cr.b = fb;
                        cr.hybrid = f;
                        cr.propability = prob[f];
                        if (origin.find(f) == origin.end()) {
                            if (fa.gene <= fb.gene) {
                                crossing_list.push_back(cr);
                                crossed.push_back(f);
                            }
                        }
                    }
                }
            }
        }
        new_origin.clear();
        for (auto f : crossed) {
            origin.insert(f);
            new_origin.insert(f);
        }
    }

    for (auto cross : crossing_list) {
        std::cout << to_string(cross.a) << " + " << to_string(cross.b);
        std::cout << " -> " << to_string(cross.hybrid);
        std::cout << " prob: " << cross.propability << std::endl;
    }
    std::cout << std::endl;
}

int main(int argc, char *argv[])
{
    std::cout << "Hello World!\n";
    auto exec_path = std::filesystem::path(argv[0]);
    auto data_path = std::filesystem::absolute(exec_path.parent_path().wstring() + L"..\\..\\..\\..\\..\\..\\data\\");
    std::wstring csv_files[] = {
        L"cosmos.csv",
        L"hyacinths.csv",
        L"lilies.csv",
        L"mums.csv",
        L"pansies.csv",
        L"roses.csv",
        L"turips.csv",
        L"windflowers.csv",
    };
    
    for (auto csv_name : csv_files) {
        auto path = std::filesystem::path(data_path.wstring() + csv_name);
        create_list(path);
    }
}
