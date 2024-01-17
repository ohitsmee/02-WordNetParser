#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <string>
#include <sstream>
#include <chrono>
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
		member = member.substr(5);
		member = member.substr(0, member.length() - 2);
		memberList[i] = member;
	}

	return memberList;
}

int main() {
	xml_document doc;
	unordered_map<string, xml_node> data;                             // map<synsetId, synset node>
	unordered_map<string, string> sub;                                // map<senseId, senseSynset>

	//const char* fileName = "environment-reference.xml";
	//const char* fileName = "environment-complete.xml";
	//const char* fileName = "gather-complete.xml";
	//const char* fileName = "gather-n-and-v.xml";
	//const char* fileName = "gathered-complete.xml";
	const char* fileName = "5-word-test.xml";


	// Load XML file
	auto start = high_resolution_clock::now();
	xml_parse_result result = doc.load_file(fileName);
	if (!result) {
		cerr << "Error loading XML file: " << result.description() << endl;
		return -1;
	}

	// Store synset id and synset node from Synset in map
	int synsetCount = 0;
	int relationCount = 0;

	xml_node root = doc.child("LexicalResource").child("Lexicon");
	for (xml_node synset = root.first_child().next_sibling("Synset"); synset; synset = synset.next_sibling("Synset")) {
		synsetCount += 2;
		string synsetId = synset.attribute("id").value();
		data[synsetId] = synset;

		// Iterate over synsets and increment counter
		for (xml_node synChild = synset.first_child(); synChild; synChild = synChild.next_sibling()) { relationCount++; }
	}

	// Store sense id and synset from LexicalEntry to map
	int lexEntryCount = 6;
	int senseCount = 0;
	int lemmaCount = 0;

	for (xml_node entry = root.first_child(); entry; entry = entry.next_sibling("LexicalEntry")) {
		lexEntryCount+=2;

		for (xml_node sense = entry.first_child().next_sibling("Sense"); sense; sense = sense.next_sibling("Sense")) {
			string senseId = sense.attribute("id").value();
			string senseSynset = sense.attribute("synset").value();
			sub[senseId] = senseSynset;

			// Iterate over Sense child nodes and increment row counter
			if (sense.first_child()) {
				senseCount+=2;
				for (xml_node srelation = sense.first_child(); srelation; srelation = srelation.next_sibling()) { senseCount++; }
			} else { senseCount++; }
		}

		// Iterate over Lemma child nodes and increment row counter
		xml_node lemma = entry.first_child();
		if (lemma.first_child()) {
			lemmaCount+=2;
			for (xml_node pronounciation = entry.first_child().first_child(); pronounciation; pronounciation = pronounciation.next_sibling()) { lemmaCount++; }
		} else { lemmaCount++; }
	}

	// Iterate over SyntacticBehaviour and increment row counter
	int behaviourCount = 0;
	for (xml_node sbehaviour = root.first_child().next_sibling("SyntacticBehaviour"); sbehaviour; sbehaviour = sbehaviour.next_sibling("SyntacticBehaviour")) { behaviourCount++; }

	// Open output CSV file
	ofstream outputFile("results.csv");

	// Iterate over LexicalEntries
	int totalWordCount = 0;
	int totalOutputRows = 0;

	for (xml_node entry = root.first_child(); entry; entry = entry.next_sibling("LexicalEntry")) {
		string wordA = entry.first_child().attribute("writtenForm").value();
		totalWordCount++;
		//cout << "WordA: " << wordA << endl;

		for (xml_node sense = entry.first_child().next_sibling("Sense"); sense; sense = sense.next_sibling("Sense")) {
			string synsetId = sense.attribute("synset").value();

			// Find entry's synset node
			auto synsetNodeIterator = data.find(synsetId);
			if (synsetNodeIterator != data.end()) {
				xml_node synsetNode = synsetNodeIterator->second;

				// Iterate over SynsetRelations
				for (xml_node srelation = synsetNode.first_child().next_sibling("SynsetRelation"); srelation; srelation = srelation.next_sibling("SynsetRelation")) {
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
							//cout << wordA << "," << relation << "," << memberList[i] << endl;
							outputFile << wordA << "," << relation << "," << memberList[i] << endl;
						}
					}
				}
			}
		}
	}

	// Find sense relation target nodes
	for (xml_node entry = root.first_child(); entry; entry = entry.next_sibling("LexicalEntry")) {
		for (xml_node sense = entry.first_child().next_sibling("Sense"); sense; sense = sense.next_sibling("Sense")) {
			string wordA = entry.first_child().attribute("writtenForm").value();

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
							//cout << wordA << "," << relationCode << "," << memberList[i] << endl;
							outputFile << wordA << "," << relationCode << "," << memberList[i] << endl;
						}
					}
				}
			}
		}
	}

	auto stop = high_resolution_clock::now();
	auto duration = duration_cast<milliseconds>(stop - start);
	int totalRowsParsed = lexEntryCount + senseCount + lemmaCount + synsetCount + relationCount + behaviourCount;

	cout << "Total rows parsed: " << totalRowsParsed << endl;
	cout << "lexEntryCount: " << lexEntryCount << endl;
	cout << "senseCount: " << senseCount << endl;
	cout << "lemmaCount: " << lemmaCount << endl;
	cout << "synsetCount: " << synsetCount << endl;
	cout << "relationCount: " << relationCount << endl;
	cout << "SyntacticBehaviour: " << behaviourCount << endl;

	outputFile << "\nTotal rows parsed: " << totalRowsParsed << endl;
	outputFile << "Total words: " << totalWordCount << endl;
	outputFile << "Total output rows: " << totalOutputRows << endl;
	outputFile << "Time spent: " << duration.count() << " ms" << endl;

	outputFile.close();
	return 0;
}