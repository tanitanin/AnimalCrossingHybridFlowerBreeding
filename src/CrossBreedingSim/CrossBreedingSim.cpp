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
    std::string white;
    std::string yellow;
    std::string red;
    std::string brightness;
    std::string color;
    std::string gene() {
        return white + yellow + red + brightness;
    }
};

bool operator==(flower_t a, flower_t b) {
    return a.gene() == b.gene();
}

bool operator<(flower_t a, flower_t b) {
    return a.gene() < b.gene();
}

std::string to_string(flower_t f) {
    std::stringstream ss;
    
    f.is_seed ? ss << "(S) " : ss << "    ";
    ss << f.white << "-" << f.yellow << "-" << f.red << "-" << f.brightness;
    ss << " (" << f.color << ")";
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

bool operator==(crossing_t a, crossing_t b) {
    std::string aa = a.a.gene() + a.b.gene() + a.hybrid.gene();
    std::string bb = b.a.gene() + b.b.gene() + b.hybrid.gene();
    return aa == bb;
}

bool operator<(crossing_t a, crossing_t b) {
    std::string aa = a.a.gene() + a.b.gene() + a.hybrid.gene();
    std::string bb = b.a.gene() + b.b.gene() + b.hybrid.gene();
    return aa < bb;
}

std::vector<std::string> cand(std::string a) {
    std::vector<std::string> cs;
    if (a.find('?') < a.length()) {
        if (a == "0?") {
            cs.push_back("00");
            cs.push_back("00");
            cs.push_back("01");
            cs.push_back("10");
        }
        else if (a == "?1") {
            cs.push_back("01");
            cs.push_back("10");
            cs.push_back("11");
            cs.push_back("11");
        }
        else if (a == "??") {
            cs.push_back("00");
            cs.push_back("00");
            cs.push_back("01");
            cs.push_back("10");
            cs.push_back("11");
            cs.push_back("11");
        }
    }
    else {
        if (a == "01") {
            cs.push_back("01");
            cs.push_back("10");
        }
        else {
            cs.push_back(a);
        }
    }
    return cs;
}

std::vector<std::string> cross(std::string a, std::string b) {
    std::set<std::string> pos_a, pos_b;
    for (auto c : cand(a)) pos_a.insert(c);
    for (auto c : cand(b)) pos_b.insert(c);
    std::vector<std::string> pos_ab;
    for (auto aa : pos_a) {
        for (auto bb : pos_b) {
            std::string ab = "??", ba = "??";
            ab[0] = aa[0]; ab[1] = bb[1];
            ba[0] = bb[0]; ba[1] = aa[1];
            pos_ab.push_back(ab);
            pos_ab.push_back(ba);
        }
    }
    std::vector<std::string> pos;
    for (auto xy : pos_ab) {
        pos.push_back(xy == "10" ? "01" : xy);
    }
    return pos;
}

std::vector<flower_t> cross(const flower_t &a, const flower_t &b, const std::map<std::string, flower_t> &gene_map) {
    std::vector<std::string> pos_w = cross(a.white, b.white);
    std::vector<std::string> pos_y = cross(a.yellow, b.yellow);
    std::vector<std::string> pos_r = cross(a.red, b.red);
    std::vector<std::string> pos_b = cross(a.brightness, b.brightness);
    std::vector<flower_t> pos;
    for (auto w : pos_w) {
        for (auto y : pos_y) {
            for (auto r : pos_r) {
                for (auto b : pos_b) {
                    flower_t f;
                    f.white = w;
                    f.yellow = y;
                    f.red = r;
                    f.brightness = b;
                    auto itr = gene_map.find(f.gene());
                    if (itr != gene_map.end()) {
                        pos.push_back(itr->second);
                    }
                }
            }
        }
    }
    return pos;
}

flower_t compose(std::vector<flower_t> flowers, std::string color) {
    std::map<std::string, int> count_y, count_r, count_w, count_b;
    for (auto f : flowers) {
        count_y[f.yellow]++;
        count_r[f.red]++;
        count_w[f.white]++;
        count_b[f.brightness]++;
    }
    flower_t f;
    f.yellow = count_y.begin()->first;
    for (auto kv : count_y) {
        if (f.yellow[0] != kv.first[0]) f.yellow[0] = '?';
        if (f.yellow[1] != kv.first[1]) f.yellow[1] = '?';
    }
    f.red = count_r.begin()->first;
    for (auto kv : count_r) {
        if (f.red[0] != kv.first[0]) f.red[0] = '?';
        if (f.red[1] != kv.first[1]) f.red[1] = '?';
    }
    f.white = count_w.begin()->first;
    for (auto kv : count_w) {
        if (f.white[0] != kv.first[0]) f.white[0] = '?';
        if (f.white[1] != kv.first[1]) f.white[1] = '?';
    }
    f.brightness = count_b.begin()->first;
    for (auto kv : count_b) {
        if (f.brightness[0] != kv.first[0]) f.brightness[0] = '?';
        if (f.brightness[1] != kv.first[1]) f.brightness[1] = '?';
    }
    f.color = color;
    return f;
}

void create_list(std::filesystem::path csv_path) {

    std::cout << "CSV: " <<  csv_path.filename() << std::endl;

    // read csv
    std::vector<flower_t> flowers;
    std::map<std::string, flower_t> flowers_index;
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
        flower.yellow = cs[1];
        flower.red = cs[2];
        flower.white = cs[3];
        flower.brightness = cs[4];
        flower.color = cs[5];
        flowers.push_back(flower);
        flowers_index[flower.gene()] = flower;
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
                        cr.a = fa < fb ? fa : fb;
                        cr.b = fa < fb ? fb : fa;
                        cr.hybrid = f;
                        cr.propability = prob[f];
                        //if (origin.find(f) == origin.end()) {
                            if (std::find(crossing_list.begin(), crossing_list.end(), cr) == crossing_list.end()) {
                                crossing_list.push_back(cr);
                                crossed.push_back(f);
                                std::cout << to_string(cr.a) << " + " << to_string(cr.b);
                                std::cout << " -> " << to_string(cr.hybrid);
                                std::cout << " prob: " << cr.propability << std::endl;
                            }
                        //}
                    }
                }
                for (auto itr : color_count) {
                    if (itr.second <= 1) continue;
                    if (itr.second > 2) continue;
                    std::string color = itr.first;
                    std::vector<flower_t> fs;
                    double sum = 0.0f;
                    for (auto itr : prob) {
                        if (itr.first.color == color) {
                            fs.push_back(itr.first);
                            sum += itr.second;
                        }
                    }
                    crossing_t cr;
                    cr.a = fa < fb ? fa : fb;
                    cr.b = fa < fb ? fb : fa;
                    cr.hybrid = compose(fs, color);
                    cr.propability = sum;
                    //if (origin.find(cr.hybrid) == origin.end()) {
                        if (std::find(crossing_list.begin(), crossing_list.end(), cr) == crossing_list.end()) {
                            crossing_list.push_back(cr);
                            crossed.push_back(cr.hybrid);
                            std::cout << to_string(cr.a) << " + " << to_string(cr.b);
                            std::cout << " -> " << to_string(cr.hybrid);
                            std::cout << " prob: " << cr.propability << std::endl;
                        }
                    //}
                }
            }
        }
        new_origin.clear();
        for (auto f : crossed) {
            origin.insert(f);
            new_origin.insert(f);
        }
    }

    //for (auto cross : crossing_list) {
    //    std::cout << to_string(cross.a) << " + " << to_string(cross.b);
    //    std::cout << " -> " << to_string(cross.hybrid);
    //    std::cout << " prob: " << cross.propability << std::endl;
    //}
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
