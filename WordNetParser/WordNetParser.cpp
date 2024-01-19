#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <string>
#include <sstream>
#include <chrono>
#include <windows.h>
#include <mutex>
#include "pugixml.hpp"

using namespace std;
using namespace pugi;
using namespace chrono;

int getRelationCode(string relation) {
	if (relation == "also")  return 0;
	else if (relation == "antonym") return 1;
	else if (relation == "attribute") return 2;

	else if (relation == "causes") return 3;

	else if (relation == "derivation") return 4;
	else if (relation == "domain_region") return 5;
	else if (relation == "domain_topic") return 6;

	else if (relation == "entails") return 7;
	else if (relation == "exemplifies") return 8;

	else if (relation == "has_domain_region") return 9;
	else if (relation == "has_domain_topic") return 10;
	else if (relation == "holo_member") return 11;
	else if (relation == "holo_part") return 12;
	else if (relation == "holo_substance") return 13;
	else if (relation == "hypernym") return 14;
	else if (relation == "hyponym") return 15;

	else if (relation == "instance_hypernym") return 16;
	else if (relation == "instance_hyponym") return 17;
	else if (relation == "is_caused_by") return 18;
	else if (relation == "is_entailed_by") return 19;
	else if (relation == "is_exemplified_by") return 20;

	else if (relation == "mero_member") return 21;
	else if (relation == "mero_part") return 22;
	else if (relation == "mero_substance") return 23;

	else if (relation == "other") return 24;

	else if (relation == "participle") return 25;
	else if (relation == "pertainym") return 26;

	else if (relation == "similar") return 27;
	return -1;
}

vector<string> getMembers(string list) {
	string members = list;
	string member;                      // store token obtained from original string
	stringstream sstream(members);
	vector<string> memberList;

	// Add individual member from members to vector
	while (getline(sstream, member, ' ')) {
		memberList.push_back(member);
	}

	// Remove unnecessary chars (oewn- and -n, -a, ...)
	for (int i = 0; i < memberList.size(); i++) {
		string member = memberList[i];
		member = member.substr(5, member.length() - 7);
		memberList[i] = member;
	}

	return memberList;
}


int main() {
	xml_document doc;
	unordered_map<string, xml_node> data;                             // map<synsetId, synset node>
	unordered_map<string, string> sub;                                // map<senseId, senseSynset>

	int parsedCounter = 13;
	int totalWordCount = 0;
	int totalOutputRows = 0;

	const char* fileName = "english-wordnet-2023.xml";

	auto start = high_resolution_clock::now();

	// Load XML file
	xml_parse_result result = doc.load_file(fileName);
	if (!result) {
		cerr << "Error loading XML file: " << result.description() << endl;
		return -1;
	}

	// Store synset id and synset node from Synset in map
	xml_node root = doc.child("LexicalResource").child("Lexicon");
	for (xml_node synset = root.child("Synset"); synset; synset = synset.next_sibling("Synset")) {
		parsedCounter += 2;
		string synsetId = synset.attribute("id").value();
		data[synsetId] = synset;

		// Iterate over synsets and increment counter
		for (xml_node synChild = synset.first_child(); synChild; synChild = synChild.next_sibling()) { parsedCounter++; }
	}

	// Store sense id and synset from LexicalEntry to map
	for (xml_node entry = root.child("LexicalEntry"); entry; entry = entry.next_sibling("LexicalEntry")) {
		parsedCounter += 2;

		for (xml_node sense = entry.child("Sense"); sense; sense = sense.next_sibling("Sense")) {
			string senseId = sense.attribute("id").value();
			string senseSynset = sense.attribute("synset").value();
			sub[senseId] = senseSynset;

			// Iterate over Sense child nodes and increment row counter
			if (sense.first_child()) {
				parsedCounter += 2;
				for (xml_node srelation = sense.first_child(); srelation; srelation = srelation.next_sibling()) { parsedCounter++; }
			} else { parsedCounter++; }
		}

		// Iterate over Lemma child nodes and increment row counter
		xml_node lemma = entry.child("Lemma");
		if (lemma. first_child()) {
			parsedCounter += 2;
			for (xml_node lchild = lemma.first_child(); lchild; lchild = lchild.next_sibling()) { parsedCounter++; }
		}
		else { parsedCounter++; }

		// Iterate over Form and increment row counter
		for (xml_node form = entry.child("Form"); form; form = form.next_sibling("Form")) { parsedCounter++; }
	}

	// Iterate over SyntacticBehaviour and increment row counter
	for (xml_node sbehaviour = root.child("SyntacticBehaviour"); sbehaviour; sbehaviour = sbehaviour.next_sibling("SyntacticBehaviour")) { parsedCounter++; }

	// Open output CSV file
	ofstream outputFile("results.csv");

	// Iterate over LexicalEntries
	for (xml_node entry = root.child("LexicalEntry"); entry; entry = entry.next_sibling("LexicalEntry")) {
		string wordA = entry.first_child().attribute("writtenForm").value();
		totalWordCount++;

		for (xml_node sense = entry.child("Sense"); sense; sense = sense.next_sibling("Sense")) {
			string synsetId = sense.attribute("synset").value();

			// Find entry's synset node
			auto synsetNodeIterator = data.find(synsetId);
			if (synsetNodeIterator != data.end()) {
				xml_node synsetNode = synsetNodeIterator->second;

				// Iterate over SynsetRelations
				for (xml_node srelation = synsetNode.child("SynsetRelation"); srelation; srelation = srelation.next_sibling("SynsetRelation")) {
					// Get target synset and relType/relationCode
					string targetSyn = srelation.attribute("target").value();
					string rtype = srelation.attribute("relType").value();
					int relation = getRelationCode(rtype);

					// Find target node
					auto targetSynsetIterator = data.find(targetSyn);
					if (targetSynsetIterator != data.end()) {
						xml_node targetNode = targetSynsetIterator->second;
						string members = targetNode.attribute("members").value();

						// Get the members and write to output file
						vector<string> memberList = getMembers(members);
						for (int i = 0; i < memberList.size(); i++) {
							totalOutputRows++;
							outputFile << wordA << "," << relation << "," << memberList[i] << endl;
						}
					}
				}
			}
		}
	}

	// Find sense relation target nodes
	for (xml_node entry = root.child("LexicalEntry"); entry; entry = entry.next_sibling("LexicalEntry")) {
		string wordA = entry.first_child().attribute("writtenForm").value();

		for (xml_node sense = entry.child("Sense"); sense; sense = sense.next_sibling("Sense")) {
			for (xml_node srelation = sense.first_child(); srelation; srelation = srelation.next_sibling("SenseRelation")) {
				int relationCode = getRelationCode(srelation.attribute("relType").value());
				string target = srelation.attribute("target").value();

				// Find corresponding sense id for sense relation target
				auto targetSenseIterator = sub.find(target);
				if (targetSenseIterator != sub.end()) {
					string targetId = targetSenseIterator->second;

					auto targetSynsetIterator = data.find(targetId);
					if (targetSynsetIterator != data.end()) {
						xml_node targetNode = targetSynsetIterator->second;
						string members = targetNode.attribute("members").value();

						// Get the members and write to output file
						vector<string> memberList = getMembers(members);
						for (int i = 0; i < memberList.size(); i++) {
							totalOutputRows++;
							outputFile << wordA << "," << relationCode << "," << memberList[i] << endl;
						}
					}
				}
			}
		}
	}

	auto stop = high_resolution_clock::now();
	auto duration = duration_cast<milliseconds>(stop - start);

	cout << "\nTotal rows parsed: " << parsedCounter << endl;
	cout << "Total words: " << totalWordCount << endl;
	cout << "Total output rows: " << totalOutputRows << endl;
	cout << "Time spent: " << duration.count() << " ms" << endl;

	outputFile << "\nTotal rows parsed: " << parsedCounter << endl;
	outputFile << "Total words: " << totalWordCount << endl;
	outputFile << "Total output rows: " << totalOutputRows << endl;
	outputFile << "Time spent: " << duration.count() << " ms" << endl;
	outputFile.close();

	return 0;
}