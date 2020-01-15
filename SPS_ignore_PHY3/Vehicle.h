#pragma once
#include <vector>
#include <random>
#include <iostream>
#include <utility>
#include <map>
#include "Logger.h"

using namespace std;

extern const int RRI;		//���M����
extern double PROB_RESOURCE_KEEP;			//���\�[�X�L�[�v�m��
extern int NUM_SUBCH;
extern const int SENSING_WINDOW;
extern const int T1;
extern const int T2;
extern Logger logger;

constexpr int C1 = 5;
constexpr int C2 = 15;

class Vehicle {
private:
	int id;				//�ԗ���id
	int subframe = 0;	//����(ms)
	int RC = -1;		//���\�[�X���Z���N�V�����J�E���^(���̏����l)

	int setRC = -1;
	int previousRC = -1;
	int prePreRC = -1;
	pair<int, int> previousLocation;
	pair<int, int> prePreLocation;
	pair<int, int> txResourceLocation;	//���ɑ��M����subframe��subCH
	vector<pair<int, int>> SB;
	random_device seed;
	mt19937 engine;
	uniform_int_distribution<> distRC, distTXTiming, distTXsubCH, distSB; //RC�̑I���C���M�^�C�~���O�̑I��,���M�T�u�`���l���̑I��
	uniform_real_distribution<> distResourceKeep;		//���\�[�X���Z���N�V�����̔���p


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

	//�Z���V���O�x�[�XSPS
	void selectResource(const vector<vector<int>>& sensingList, int subframe) {
		wasRCComplete = true;

		prePreRC = previousRC;
		previousRC = setRC;
		RC = distRC(engine);
		setRC = RC;

		prePreLocation = previousLocation;
		previousLocation = txResourceLocation;

		multimap<int, pair<int, int>> map;

		//probResouceKeep�̔���
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

			//���20%�Ɉʒu����L�[���傫���L�[�������ʒu���v�Z
			auto border = distance(map.begin(), map.upper_bound(next(map.begin(), (int)ceil(((T2 - T1) * NUM_SUBCH) * 0.2))->first));
			uniform_int_distribution<>::param_type paramSB(0, border - 1);
			distSB.param(paramSB);
			auto nextResouceLocation = next(map.begin(), distSB(engine));

			txResourceLocation.first = nextResouceLocation->second.first + subframe;
			txResourceLocation.second = nextResouceLocation->second.second;
			SB.clear();

		//probResouceKeep�̔���
		//if (distResourceKeep(engine) > PROB_RESOURCE_KEEP) {
		//	isReselection = true;
		//	for (auto itr_subFrame = sensingList.begin() + 1; itr_subFrame != sensingList.end() - 1; itr_subFrame++) {
		//		//itr_subCH_init:���g�p�T�u�`���l�������̎n�_
		//		for (auto itr_subCH = itr_subFrame->begin(); itr_subCH != itr_subFrame->end(); itr_subCH++) {
		//			if (*itr_subCH == 0) {
		//				//�T�u�`���l�����󂢂Ă����ꍇ�C
		//				SB.emplace_back(make_pair(distance(sensingList.begin(), itr_subFrame), distance(itr_subFrame->begin(), itr_subCH)));
		//			}
		//		}
		//	}

		//	//SB���烉���_���Ɉ���M���\�[�X��I��
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

	//RC=0�ƂȂ�Ȃ��ꍇ
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

	//�R���X�g���N�^
	Vehicle(int id) {
		//�v���C�x�[�g�ϐ��ɑ��
		this->id = id;
		engine.seed(seed());
		//engine.seed(id);

		//RC�̐ݒ�
		uniform_int_distribution<>::param_type paramRC(C1, C2);
		distRC.param(paramRC);
		RC = distRC(engine);
		setRC = RC;

		//������ݒ�
		uniform_int_distribution<>::param_type paramTXTiming(0, RRI - 1);			//���Ԏ������̐ݒ�
		uniform_int_distribution<>::param_type paramTXSubCH(0, NUM_SUBCH - 1);		//���g�������̐ݒ�
		uniform_real_distribution<>::param_type paramResourceKeep(0, 1);			//���\�[�X�đI�����p
		distTXTiming.param(paramTXTiming);
		distTXsubCH.param(paramTXSubCH);
		distResourceKeep.param(paramResourceKeep);

		//������ԂŃ����_���ɑ��M����T�u�t���[���C�T�u�`���l��������
		txResourceLocation.first = distTXTiming(engine);
		txResourceLocation.second = distTXsubCH(engine);
	}
};