#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <vector>
#include <numeric>
#include <unordered_map>
#include <algorithm>
#include <map>
#include <set>
#include "Vehicle.h"
#include "Logger.h"
//#define CON_COL
//#define LOG
//#define PROPOSAL

#ifdef LOG
Logger logger;
#endif

using namespace std;

const int RRI = 100;
const int SENSING_WINDOW = 1000;			//初期状態で送信タイミングを決定する範囲
int NUM_SUBCH;
double PROB_RESOURCE_KEEP;
const int T1 = 1;					//選択ウィンドウT1
const int T2 = 100;					//選択ウィンドウT2

struct HashPair {
	template <class T1, class T2>
	size_t operator()(const pair < T1, T2>& p)const {
		auto hash1 = hash<T1>{}(p.first);
		auto hash2 = hash<T2>{}(p.second);

		size_t seed = 0;
		seed ^= hash1 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= hash2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		return seed;
	}
};

int main() {
	constexpr int warmupTime = 1500;			//から回し時間
	constexpr int simTime = (100 * 100) + warmupTime;			//全体のシミュレーション時間(ms)

	constexpr int minVehicle = 50;		//最小車両数
	constexpr int maxVehicle = 200;		//最大車両数
	constexpr int numTrial = 1000;		//各車両数での試行回数



	int subframe = 0;					//1ms単位のシミュレーション時刻
	int timeGap = 0;					//nextEventまでの差
	int nextEventSubframe;				//次のイベントのシミュレーション時刻

	cout << "SPS simulator" << endl;
	cout << "######<intput parameter>######" << endl;
	cout << "reselection probability << "; cin >> PROB_RESOURCE_KEEP;
	cout << "number of subCH << "; cin >> NUM_SUBCH;

	vector<Vehicle*> vehicleList;		//車両を格納するvector
	vector<vector<int>> sensingList(SENSING_WINDOW + 1, vector<int>(NUM_SUBCH, 0));		//センシングリスト

	//実行時間関係の設定
	chrono::system_clock::time_point mstart, mend;
	mstart = chrono::system_clock::now();

#ifdef PROPOSAL
	ofstream output("p" + to_string((int)(PROB_RESOURCE_KEEP * 10)) + "_" + to_string(NUM_SUBCH) + "ch_proposal.csv");
#else
	ofstream output("p" + to_string((int)(PROB_RESOURCE_KEEP * 10)) + "_" + to_string(NUM_SUBCH) + "ch.csv");
#endif // PROPOSAL



	//車両数のループ
	for (int numVehicle = minVehicle; numVehicle <= maxVehicle; numVehicle += 25) {
		cout << numVehicle << endl;
		int sendCounter = 0;						//送信したパケット数
		int collisionCounter = 0;					//衝突したパケット数

		map<int, int> resultContinuous;				
		map<int, int> resultColVehicles;

		//試行回数分ループ
		for (int trial = 0; trial < numTrial; trial++) {
			//subframe初期化
			subframe = 0;
			unordered_map<pair<int, int>, int, HashPair> collisionPairs;
			bool warmupFlag = false;

			//車両インスタンスの生成
			for (int i = 0; i < numVehicle; i++) {
				vehicleList.emplace_back(new Vehicle(i));
			}

			//subframeのループ，直近の送信予定subframeまで進める
			while (subframe < simTime) {
#ifdef LOG
				logger.writeSubframe(subframe);
#endif
				set<int> RCset;

				//subCH毎に送信を行う車両のidを格納
				vector<Vehicle*> txVehicles;
				vector<vector<Vehicle*>> txVehiclesPerSubCH(NUM_SUBCH, vector<Vehicle*>());


				if (subframe != 0) {
					//sensingList更新
					sensingList.assign(sensingList.begin() + timeGap, sensingList.end());
					int gapSize = SENSING_WINDOW - sensingList.size() + 1;
					sensingList.insert(sensingList.end(), gapSize, vector<int>(NUM_SUBCH, 0));
				}

				//送信を行う車両の動作
				for (Vehicle*& vehicle : vehicleList) {
					//送信を行う車両の発見
					if (subframe == vehicle->getTxResourceLocation().first) {
						RCset.insert(vehicle->getRC());
						txVehicles.emplace_back(vehicle);
						txVehiclesPerSubCH[vehicle->getTxResourceLocation().second].emplace_back(vehicle);
						if (warmupTime < subframe)
							sendCounter++;

						//送信に使用したサブチャネルをマーク
							//sensingListの最後尾が現時刻のsubframeなのでsensingWindow番目に記録
						sensingList[SENSING_WINDOW][vehicle->getTxResourceLocation().second] = 1;

						//RCの減算
						vehicle->decrementRC();
#ifdef LOG
						logger.writeSend(vehicle->getId(), vehicle->getTxResourceLocation(), vehicle->getRC());
#endif
					}
				}

				if (!warmupFlag && warmupTime <= subframe) {
					warmupFlag = true;
					for (Vehicle* vehicle : vehicleList) {
						vehicle->resetRC();
					}
				}
#ifdef CON_COL
				if (warmupTime <= subframe) {
					for (vector<int> vec : txIdPerSubCH) {
						if (vec.size() > 0) {
							auto itr = collisionPairs.begin();
							if (vec.size() == 1) {
								while (itr != collisionPairs.end()) {
									if (itr->first.first == vec[0] || itr->first.second == vec[0]) {
										if (itr->second < 5) {
											exit(-2);
										}
										resultContinuous[itr->second]++;
										collisionPairs.erase(itr++);
									}
									else ++itr;
								}
							}
							else {
								resultColVehicles[vec.size()]++;
								while (itr != collisionPairs.end()) {
									if ((find(vec.begin(), vec.end(), itr->first.first) == vec.end() && find(vec.begin(), vec.end(), itr->first.second) != vec.end())
										|| (find(vec.begin(), vec.end(), itr->first.first) != vec.end() && find(vec.begin(), vec.end(), itr->first.second) == vec.end())) {
										if (itr->second < 5) {
											exit(-2);
										}
										resultContinuous[itr->second]++;
										collisionPairs.erase(itr++);
									}
									else ++itr;
								}

								sort(vec.begin(), vec.end());
								for (auto itr1 = vec.begin(); itr1 != vec.end(); ++itr1) {
									for (auto itr2 = vec.end() - 1; itr2 != itr1; --itr2) {
										collisionPairs[make_pair(*itr1, *itr2)]++;
									}
								}
							}
						}
					}
				}
#endif

				for (auto&& vec : txVehiclesPerSubCH) {
					if (warmupTime <= subframe && vec.size() > 1) {
						collisionCounter += vec.size();
#ifdef LOG
						logger.writeCollision(vec[0]->getTxResourceLocation().second);
#endif 
					}
					for (auto&& vehicle : vec) {
						//RCが0になったら再選択
						if (vehicle->getRC() <= 0) {
							//再選択で使用するselectionListを設定
							//リソースの再選択判定
							vehicle->selectResource(sensingList, subframe);
						}
						//RCが0出ない場合は送信スケジュールをRRI分更新
						else
							vehicle->txResourceUpdate();
					}
				}
#ifdef CON_COL
				for (auto&& vec : txVehiclesPerSubCH) {
					for (auto&& vehicle : vec) {
						if (vehicle->wasRCComplete)
							initCollisionCounter++;
					}
				}


				for (auto&& elem : collisionPairs) {
					for (auto&& vec : txIdPerSubCH) {
						if (find(vec.begin(), vec.end(), elem.first.first) != vec.end() && find(vec.begin(), vec.end(), elem.first.second) != vec.end()) {
							auto target1 = vehicleList[*find(vec.begin(), vec.end(), elem.first.first)];
							auto target2 = vehicleList[*find(vec.begin(), vec.end(), elem.first.second)];
							if (target1->isReselection && target2->isReselection) {
								if (target1->getTxResourceLocation() == target2->getTxResourceLocation()) {
									reselectionAndCollisionCounter++;
								}
							}
							else if (target1->isNotReselection)
								noReselectionAndCollisionCounter++;
							else if (target2->isNotReselection)
								noReselectionAndCollisionCounter++;
						}
					}
				}
#endif CON_COL

				//次イベント時間の設定
				nextEventSubframe = INT_MAX;	//仮の初期値
				//直近のイベント時間を見つける
				for (Vehicle* vehicle : vehicleList) {
#ifdef PROPOSAL
					if (find(txVehicles.begin(), txVehicles.end(), vehicle) == txVehicles.end()) {
						if (find(RCset.begin(), RCset.end(), vehicle->getRC() + 1) != RCset.end()) {
							vehicle->changeRC();
						}
					}
#endif
					auto comparison = vehicle->getTxResourceLocation().first;
					nextEventSubframe = nextEventSubframe > comparison ? comparison : nextEventSubframe;
				}
				//現在の時間と次のイベントまでの時間差
				timeGap = nextEventSubframe - subframe;

				//時刻更新
				subframe = nextEventSubframe;
#ifdef LOG
				logger.writeSubframeTerm();
#endif
			}

			//車両インスタンスの削除
			for (Vehicle* vehicle : vehicleList) {
				delete vehicle;
			}
			vehicleList.clear();
#ifdef LOG
			logger.writeTerm();
#endif
		}

		output << numVehicle << "," << (double)collisionCounter / (double)sendCounter << endl;
		cout << (double)collisionCounter / (double)sendCounter << endl;
	}
	output.close();
	mend = chrono::system_clock::now();
	std::cout << endl << "実行時間 " << chrono::duration_cast<chrono::minutes>(mend - mstart).count() << " m" << endl;
	return 0;
}