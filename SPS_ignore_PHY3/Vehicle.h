#pragma once
#include <vector>
#include <random>
#include <iostream>
#include <utility>
#include <map>
#include "Logger.h"

using namespace std;

constexpr int RRI = 100;		//送信周期
constexpr double probResourceKeep = 0.0;				//リソースキープ確率

extern Logger logger;

class Vehicle {
private:
	int id;				//車両のid
	int subframe = 0;	//時刻(ms)
	int numSubCH;		//総サブチャネル数
	int txSubCHLength;	//送信に使用するサブチャネル数
	int RC = -1;		//リソースリセレクションカウンタ(仮の初期値)
	int C1, C2;			//RCの範囲
	int txTimingRange;	//初期状態で送信タイミングを決定する範囲

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
	void selectResource(vector<vector<int>>& candidateResource, int subframe) {
		wasRCComplete = true;

		prePreRC = previousRC;
		previousRC = setRC;
		RC = distRC(engine);
		setRC = RC;

		prePreLocation = previousLocation;
		previousLocation = txResourceLocation;

		//probResouceKeepの判定
		if (distResourceKeep(engine) > probResourceKeep) {
			isReselection = true;
			for (auto itr_subFrame = candidateResource.begin() + 1; itr_subFrame != candidateResource.end() - 1; itr_subFrame++) {
				//itr_subCH_init:未使用サブチャネル検索の始点
				for (auto itr_subCH = itr_subFrame->begin(); itr_subCH != itr_subFrame->end(); itr_subCH++) {
					if (*itr_subCH == 0) {
						//サブチャネルが空いていた場合，
						SB.emplace_back(make_pair(distance(candidateResource.begin(), itr_subFrame), distance(itr_subFrame->begin(), itr_subCH)));
					}
				}
			}

			//SBからランダムに一つ送信リソースを選択
			uniform_int_distribution<>::param_type paramSB(0, SB.size() - 1);
			distSB.param(paramSB);
			txResourceLocation = SB[distSB(engine)];
			txResourceLocation.first += subframe;
			SB.clear();
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
	Vehicle(int id, int subCH, int txSubCH) {
		//プライベート変数に代入
		this->id = id;
		numSubCH = subCH;
		txSubCHLength = txSubCH;
		txTimingRange = 100;

		//RC設定範囲を定義
		if (RRI >= 100) {
			C1 = 5;
			C2 = 15;
		}
		else if (RRI == 50) {
			C1 = 10;
			C2 = 30;
		}
		else if (RRI == 20) {
			C1 = 25;
			C2 = 75;
		}
		else {
			cout << "RRI error" << endl;
			exit(1);
		}

		engine.seed(seed());
		//engine.seed(id);


		//RCの決定
		uniform_int_distribution<>::param_type paramRC(C1, C2);
		distRC.param(paramRC);
		RC = distRC(engine);
		setRC = RC;

		//送信リソースの決定
		uniform_int_distribution<>::param_type paramTXTiming(0, txTimingRange - 1);				//時間軸方向の設定
		uniform_int_distribution<>::param_type paramTXSubCH(0, numSubCH - txSubCHLength);		//周波数方向の設定
		uniform_real_distribution<>::param_type paramResourceKeep(0, 1);						//RCの初期値

		distTXTiming.param(paramTXTiming);
		distTXsubCH.param(paramTXSubCH);
		distResourceKeep.param(paramResourceKeep);

		//初期状態でランダムに送信するサブフレーム，サブチャネルを決定
		txResourceLocation.first = distTXTiming(engine);
		txResourceLocation.second = distTXsubCH(engine);
	}
};