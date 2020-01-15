#pragma once
#include <vector>
#include <random>
#include <iostream>
#include <utility>
#include <map>
#include "Logger.h"

using namespace std;

extern const int RRI;		//送信周期
extern double PROB_RESOURCE_KEEP;			//リソースキープ確率
extern int NUM_SUBCH;
extern const int SENSING_WINDOW;
extern const int T1;
extern const int T2;
extern Logger logger;

constexpr int C1 = 5;
constexpr int C2 = 15;

class Vehicle {
private:
	int id;				//車両のid
	int subframe = 0;	//時刻(ms)
	int RC = -1;		//リソースリセレクションカウンタ(仮の初期値)

	int setRC = -1;
	int previousRC = -1;
	int prePreRC = -1;
	pair<int, int> previousLocation;
	pair<int, int> prePreLocation;
	pair<int, int> txResourceLocation;	//次に送信するsubframeとsubCH
	vector<pair<int, int>> SB;
	random_device seed;
	mt19937 engine;
	uniform_int_distribution<> distRC, distTXTiming, distTXsubCH, distSB; //RCの選択，送信タイミングの選択,送信サブチャネルの選択
	uniform_real_distribution<> distResourceKeep;		//リソースリセレクションの判定用


public:
	bool isReselection = false;
	bool isNotReselection = false;
	bool wasRCComplete = false;
	int getId() {
		return id;
	}

	int getRC() {
		return RC;
	}

	void decrementRC() {
		RC--;
	}

	int getSubframe() {
		return subframe;
	}


	pair<int, int> getTxResourceLocation() {
		return txResourceLocation;
	}

	void printResourceRocation() {
		cout << "subCHIndex " << txResourceLocation.first << ", subCHLength " << txResourceLocation.second << endl;
	}

	//センシングベースSPS
	void selectResource(const vector<vector<int>>& sensingList, int subframe) {
		wasRCComplete = true;

		prePreRC = previousRC;
		previousRC = setRC;
		RC = distRC(engine);
		setRC = RC;

		prePreLocation = previousLocation;
		previousLocation = txResourceLocation;

		multimap<int, pair<int, int>> map;

		//probResouceKeepの判定
		if (distResourceKeep(engine) > PROB_RESOURCE_KEEP) {
			isReselection = true;
			for (int i = 1; i < RRI; i++) {
				for (int j = 0; j < NUM_SUBCH; j++) {
					int sum = 0;
					for (int k = 0; k < 10; k++) {
						sum += sensingList[i + (100 * k)][j];
					}
					map.insert(make_pair(sum, make_pair(i, j)));
				}
			}

			//上位20%に位置するキーより大きいキーが現れる位置を計算
			auto border = distance(map.begin(), map.upper_bound(next(map.begin(), (int)ceil(((T2 - T1) * NUM_SUBCH) * 0.2))->first));
			uniform_int_distribution<>::param_type paramSB(0, border - 1);
			distSB.param(paramSB);
			auto nextResouceLocation = next(map.begin(), distSB(engine));

			txResourceLocation.first = nextResouceLocation->second.first + subframe;
			txResourceLocation.second = nextResouceLocation->second.second;
			SB.clear();

		//probResouceKeepの判定
		//if (distResourceKeep(engine) > PROB_RESOURCE_KEEP) {
		//	isReselection = true;
		//	for (auto itr_subFrame = sensingList.begin() + 1; itr_subFrame != sensingList.end() - 1; itr_subFrame++) {
		//		//itr_subCH_init:未使用サブチャネル検索の始点
		//		for (auto itr_subCH = itr_subFrame->begin(); itr_subCH != itr_subFrame->end(); itr_subCH++) {
		//			if (*itr_subCH == 0) {
		//				//サブチャネルが空いていた場合，
		//				SB.emplace_back(make_pair(distance(sensingList.begin(), itr_subFrame), distance(itr_subFrame->begin(), itr_subCH)));
		//			}
		//		}
		//	}

		//	//SBからランダムに一つ送信リソースを選択
		//	uniform_int_distribution<>::param_type paramSB(0, SB.size() - 1);
		//	distSB.param(paramSB);
		//	txResourceLocation = SB[distSB(engine)];
		//	txResourceLocation.first += subframe;
		//	SB.clear();

#ifdef LOG
			logger.writeReselection(id, txResourceLocation, RC);
#endif
		}
		else {
			isNotReselection = true;
			txResourceLocation.first += RRI;
#ifdef LOG
			logger.writeKeep(id, txResourceLocation, RC);
#endif
	}
}

	//RC=0とならない場合
	void txResourceUpdate() {
		isReselection = false;
		isNotReselection = false;
		wasRCComplete = false;
		txResourceLocation.first += RRI;
	}

	void changeRC() {
		uniform_int_distribution<> dist(-5, 5);
		int tmp = RC;
		previousRC = setRC;
		RC += dist(engine);
		setRC = RC;
#ifdef LOG
		logger.writeRCChange(id, RC, tmp);
#endif
	}

	void resetRC() {
		previousRC = setRC;
		RC = distRC(engine);
		setRC = RC;
	}

	//コンストラクタ
	Vehicle(int id) {
		//プライベート変数に代入
		this->id = id;
		engine.seed(seed());
		//engine.seed(id);

		//RCの設定
		uniform_int_distribution<>::param_type paramRC(C1, C2);
		distRC.param(paramRC);
		RC = distRC(engine);
		setRC = RC;

		//乱数器設定
		uniform_int_distribution<>::param_type paramTXTiming(0, RRI - 1);			//時間軸方向の設定
		uniform_int_distribution<>::param_type paramTXSubCH(0, NUM_SUBCH - 1);		//周波数方向の設定
		uniform_real_distribution<>::param_type paramResourceKeep(0, 1);			//リソース再選択時用
		distTXTiming.param(paramTXTiming);
		distTXsubCH.param(paramTXSubCH);
		distResourceKeep.param(paramResourceKeep);

		//初期状態でランダムに送信するサブフレーム，サブチャネルを決定
		txResourceLocation.first = distTXTiming(engine);
		txResourceLocation.second = distTXsubCH(engine);
	}
};