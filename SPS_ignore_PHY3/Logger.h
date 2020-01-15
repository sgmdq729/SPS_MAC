#pragma once
#include <fstream>
#include <time.h>
#include <sstream>
#include <tuple>
#include <iostream>
#include <iomanip>

using namespace std;

class Logger {
private:
	ofstream log;
	time_t timer;
	struct tm* local;

public:
	Logger() {
		timer = time(NULL);
		local = localtime(&timer);
		ostringstream ss;
		ss << (local->tm_year + 1900) % 2000 << setw(2) << setfill('0') << local->tm_mon + 1 << setw(2) << setfill('0') << local->tm_mday << "_" << setw(2) << setfill('0') << local->tm_hour << "'" << setw(2) << setfill('0') << local->tm_min << "''" << setw(2) << setfill('0') << local->tm_sec << ".xml";
		log.open(ss.str());
	}
	~Logger() {
		log.close();
	}
	void writeSettings(int simTime, int warmupTime, int numVehicle, int RRI, int numSubCH, int txNumSubCH, int T1, int T2, double resourceKeep) {
		log << "<!-- generated on " << local->tm_year + 1900 << "/" << local->tm_mon + 1 << "/" << local->tm_mday << " " << local->tm_hour << ":" << local->tm_min << ":" << local->tm_sec << endl;
		log << "<paramater settings>" << endl;
		log << "    simTime=" << simTime << endl;
		log << "    warmupTime=" << warmupTime << endl;
		log << "    numVehicle=" << numVehicle << endl;
		log << "    RRI=" << RRI << endl;
		log << "    numSubCH=" << numSubCH << endl;
		log << "    txNumSubCH=" << txNumSubCH << endl;
		log << "    T1,T2=" << T1 << "," << T2 << endl;
		log << "    resourceKeep=" << resourceKeep << endl;
		log << "</pramater settings>" << endl << "-->" << endl << endl;
	}
	void writeSubframe(int subframe) {
		log << "<timestep subframe=\"" << subframe << "\">" << endl;
	}
	void writeSubframeTerm() {
		log << "</timestep>" << endl;
	}
	void writeTerm() {
		log << "simulation end" << endl;
	}
	void writeSend(int id, pair<int, int> txResourceLocation, int RC) {
		log << "    <id=" << left << setw(4) << id << "send sucCHIndex=\"" << txResourceLocation.second << "\" RC=\"" << RC + 1 << "to" << RC << "\"/>" << endl;
	}
	void writeRC(int id, int RC) {
		log << "    <id=" << left << setw(4) << id << "RC=\"" << RC << "\"/>" << endl;
	}
	void writeReselection(int id, pair<int, int> txResourceLocation, int RC) {
		log << "    <id=" << left << setw(4) << id << "resource reselection" << " next send subframe=\"" << txResourceLocation.first << "\" subCHIndex=\"" << txResourceLocation.second << "\" new RC=\"" << RC << "\"/>" << endl;
	}
	void writeKeep(int id, pair<int, int> txResourceLocation, int RC) {
		log << "    <id=" << left << setw(4) << id << "resource keep" << " next send subframe=\"" << txResourceLocation.first << "\" subCHIndex=\"" << txResourceLocation.second << "\" new RC=\"" << RC << "\"/>" << endl;
	}
	void writeCollision(int subCH) {
		log << "    <collision subCHIndex=\"" << subCH << "\"/>" << endl;
	}
	void writeRCChange(int id, int newRC, int oldRC) {
		log << "    <id=" << left << setw(4) << id << "change RC \"" << oldRC << "to" << newRC << "\"/>" << endl;
	}

	void directWrite(string str) {
		log << str;
	}
	void writeEndl() {
		log << endl;
	}
};