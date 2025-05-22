#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include "../TST.hpp"

typedef int ValueType;

int main() {

    std::ifstream file("../DATASETS/DSSN.txt");
    if (!file) {
        std::cerr << "Error: Could not open the file!" << std::endl;
        return 1;
    }

    std::string line;
    TST::TST<ValueType> tst(20, "hour");
    double cumulative_encoding_time = 0, cumulative_insertion_time = 0;
    int year, month, day, hour; // Temporal
    double latitude, longitude; // Spatial
    int lineNum = 0; // Data
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        char comma;

        ss >> year >> comma >> month >> comma >> day >> comma >> hour >> comma >> latitude >> comma >> longitude;

        /* Encoding */
        auto start_encoding = std::chrono::system_clock::now();
        unsigned long long encoded_temp_insert = tst.time_encoder(year, month, day, hour);
        unsigned long long encoded_spat_insert = tst.space_encoder(latitude, longitude);
        auto end_encoding = std::chrono::system_clock::now();
        cumulative_encoding_time += std::chrono::duration<double, std::milli>(end_encoding - start_encoding).count();

        auto start_insertion = std::chrono::system_clock::now();
        tst.Insert(encoded_temp_insert, encoded_spat_insert, ++lineNum);
        auto end_insertion = std::chrono::system_clock::now();
        cumulative_insertion_time += std::chrono::duration<double, std::milli>(end_insertion - start_insertion).count();
    }

    std::cout << "====== DSSN: Trie Construction =====" << std::endl;
    std::cout << ">> Data Encoding Elapsed Time: " << cumulative_encoding_time << " ms" << std::endl;
    std::cout << ">> Index Building Elapsed Time (Node Insertion + Data Pointing): " << cumulative_insertion_time << " ms" << std::endl;
    std::cout << "    # of Internal Nodes: " << tst.getInter_NodeCount() << std::endl;
    std::cout << "    # of Leaf Nodes: " << tst.getLeaf_NodeCount() << std::endl;
    std::cout << "    # of Total Tree Nodes: " << tst.getTotal_NodeCount() << std::endl << std::endl;
    file.close();

    std::cout << "====== DSSN: Node Deletion =====" << std::endl;
    std::cout << ">> Delete Last IoT Data Record" << std::endl;

    std::cout << "	# of IoT Data (Before Deletion): " << tst.get_DataCount() << std::endl;
    auto encoded_temp_delete = tst.time_encoder(year, month, day, hour);
    auto encoded_spat_delete = tst.space_encoder(latitude, longitude);
    tst.Delete(encoded_temp_delete, encoded_spat_delete, lineNum);
    std::cout << "	# of IoT Data (After Deletion): " << tst.get_DataCount() << std::endl << std::endl;

    std::cout << "====== DSSN: Query Execution =====" << std::endl;
    std::vector<ValueType> result;
    int nhits;

    // Example Query 1:
    unsigned long long timeWindow_start1 = tst.time_encoder(2016, 4, 21, 0);
    unsigned long long timeWindow_end1 = tst.time_encoder(2016, 4, 21, 1);
    std::vector<double> spatialWindow_leftBottom1 = {23.176, 72.630};
    std::vector<double> spatialWindow_rightUpper1 = {23.210, 72.635};

    auto s2Cells1 = tst.REC_S2_FINDER(spatialWindow_leftBottom1, spatialWindow_rightUpper1);
    tst.range_search(s2Cells1, timeWindow_start1, timeWindow_end1, result);
    nhits = result.size();
    std::cout << "Example Query 1: " << nhits << " results found." << std::endl;

    // Example Query 2:
    unsigned long long timeWindow_start2 = tst.time_encoder(2016, 4, 21, 0);
    unsigned long long timeWindow_end2 = tst.time_encoder(2016, 4, 21, 6);
    std::vector<double> spatialWindow_leftBottom2 = {23.178, 72.632};
    std::vector<double> spatialWindow_rightUpper2 = {23.190, 72.645};

    auto s2Cells2 = tst.REC_S2_FINDER(spatialWindow_leftBottom2, spatialWindow_rightUpper2);
    tst.range_search(s2Cells2, timeWindow_start2, timeWindow_end2, result);
    nhits = result.size();
    std::cout << "Example Query 2: " << nhits << " results found." << std::endl;

    // Example Query 3:
    unsigned long long timeWindow_start3 = tst.time_encoder(2016, 4, 21, 0);
    unsigned long long timeWindow_end3 = tst.time_encoder(2016, 4, 21, 12);
    std::vector<double> spatialWindow_leftBottom3 = {23.174, 72.628};
    std::vector<double> spatialWindow_rightUpper3 = {23.205, 72.654};

    auto s2Cells3 = tst.REC_S2_FINDER(spatialWindow_leftBottom3, spatialWindow_rightUpper3);
    tst.range_search(s2Cells3, timeWindow_start3, timeWindow_end3, result);
    nhits = result.size();
    std::cout << "Example Query 3: " << nhits << " results found." << std::endl;

    return 0;
}
