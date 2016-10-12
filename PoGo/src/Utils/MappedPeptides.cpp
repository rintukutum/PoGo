#include "MappedPeptides.h"


MappedPeptides::MappedPeptides(void) :
	m_mapping(),
	m_count_peptides(0),
	m_tissuemap(std::map<std::string, unsigned int>()),
	m_tissueindex(0) {}
MappedPeptides::~MappedPeptides(void) {}

void MappedPeptides::add_gene_from_gtf(std::string const& gtfGeneLine) {
	GeneEntry* gene = new GeneEntry(gtfGeneLine);
	m_mapping.insert(std::pair<std::string, MapEntry*>(gene->get_id(), new MapEntry(gene)));
}

void MappedPeptides::add_transcript_id_to_gene(std::string const& gtftranscriptline) {
	std::string transcript_id = GeneEntry::extract_transcript_id(gtftranscriptline);
	std::string gene_id = GeneEntry::extract_gene_id(gtftranscriptline);

	if (m_mapping.count(gene_id) > 0) {
		m_mapping[gene_id]->add_transcript_id(transcript_id);
	} 
}

void MappedPeptides::to_gtf(const std::string& filename, const std::string& source) {
	std::ofstream ofs;
	ofs.open(filename.c_str());
	to_gtf(source, ofs);
	ofs.close();
}

void MappedPeptides::to_bed(std::string filename) {
	std::ofstream ofs;
	ofs.open(filename.c_str());
	to_bed(ofs);
	ofs.close();
}

void MappedPeptides::to_gct(std::string filename) {
	std::ofstream ofs;
	ofs.open(filename.c_str());
	to_gct(ofs);
	ofs.close();
}

void MappedPeptides::to_ptmbed(std::string filename) {
	std::ofstream ofs;
	ofs.open(filename.c_str());
	to_ptmbed(ofs);
	ofs.close();
}

void MappedPeptides::to_gtf(std::string source, std::ostream& os) {
	std::set<MapEntry*, mapentry_p_compare> mapping_set;

	for (std::map<std::string, MapEntry*>::iterator it = m_mapping.begin(); it != m_mapping.end(); ++it) {
		mapping_set.insert(it->second);
	}

	for (std::set<MapEntry*, mapentry_p_compare>::iterator it = mapping_set.begin(); it != mapping_set.end(); ++it) {
		(*it)->to_gtf(source, os);
	}
}

void MappedPeptides::to_bed(std::ostream& os) {
	std::set<MapEntry*, mapentry_p_compare> mapping_set;

	for (std::map<std::string, MapEntry*>::iterator it = m_mapping.begin(); it != m_mapping.end(); ++it) {
		mapping_set.insert(it->second);
	}

	for (std::set<MapEntry*, mapentry_p_compare>::iterator it = mapping_set.begin(); it != mapping_set.end(); ++it) {
		(*it)->to_bed(os);
	}
}

void MappedPeptides::to_gct(std::ostream& os) {
	std::set<MapEntry*, mapentry_p_compare> mapping_set;

	os << "#1.2\t";
	for (unsigned int i = 0; i < m_tissuemap.size(); ++i) {
		os << "\t";
	}
	os << "\n" << m_count_peptides << "\t" << m_tissuemap.size();
	for (unsigned int i = 0; i < m_tissuemap.size(); ++i) {
		os << "\t";
	}
	std::string tissue_string = tissuemap_to_sorted_string("\t");
	os << "\nName\tDescription" << tissue_string << "\n";

	std::vector<std::string> tokens = std::vector<std::string>();
	tokenize(tissue_string, tokens, "\t", false);

	for (std::map<std::string, MapEntry*>::iterator it = m_mapping.begin(); it != m_mapping.end(); ++it) {
		mapping_set.insert(it->second);
	}

	for (std::set<MapEntry*, mapentry_p_compare>::iterator it = mapping_set.begin(); it != mapping_set.end(); ++it) {
		(*it)->to_gct(tokens, os);
	}
}

void MappedPeptides::to_ptmbed(std::ostream& os) {
	std::set<MapEntry*, mapentry_p_compare> mapping_set;

	for (std::map<std::string, MapEntry*>::iterator it = m_mapping.begin(); it != m_mapping.end(); ++it) {
		mapping_set.insert(it->second);
	}

	for (std::set<MapEntry*, mapentry_p_compare>::iterator it = mapping_set.begin(); it != mapping_set.end(); ++it) {
		(*it)->to_ptmbed(os);
	}
}

void MappedPeptides::remove_all_peptides() {
	for (std::map<std::string, MapEntry*>::iterator it = m_mapping.begin(); it != m_mapping.end(); ++it) {
		it->second->remove_peptides();
	}
	m_tissuemap.clear();
	m_count_peptides = 0;
}

std::string MappedPeptides::tissuemap_to_sorted_string(const std::string& sep) {
	std::set<std::pair<std::string, unsigned int>, byIntValue> tissueset;
	for (std::map<std::string, unsigned int>::iterator it = m_tissuemap.begin(); it != m_tissuemap.end(); ++it) {
		tissueset.insert(std::pair<std::string, unsigned int>(it->first, it->second));
	}

	std::stringstream ss;
	for (std::set<std::pair<std::string, unsigned int>, byIntValue>::iterator it = tissueset.begin(); it != tissueset.end(); ++it) {
		ss << "\t" << it->first;
	}
	return ss.str();
}

void MappedPeptides::add_peptide(CoordinateWrapper& coordwrapper, const std::string& sequence, const std::string& tag, unsigned int sigPSMs, unsigned int genes, std::ofstream& ofstream, double quant, gene_id_map_t::value_type const& transcriptsEntry) {
	if (m_tissuemap.count(tag) == 0) {
		m_tissuemap.insert(std::pair<std::string, unsigned int>(tag, m_tissueindex));
		++m_tissueindex;
	}

	std::string const& geneID = transcriptsEntry.first;

	if (m_mapping.count(geneID) == 0) {
		std::stringstream ss;
		for (transcripts_t::transcript_id_map_t::const_iterator entryIt = transcriptsEntry.second.m_entries.begin(); entryIt != transcriptsEntry.second.m_entries.end(); ++entryIt) {
			if (entryIt != transcriptsEntry.second.m_entries.begin()) {
				ss << ",";
			}
			ss << entryIt->first;
		}
		ofstream << geneID << "\t" << sequence << "\t" << ss.str() << "\t" << genes << "\t" << tag << "\t" << sigPSMs << "\t" << quant << "\n";
	} else if (m_mapping.count(geneID) > 0) {
		m_count_peptides += m_mapping[geneID]->add_peptide(coordwrapper, sequence, tag, sigPSMs, genes, ofstream, quant, transcriptsEntry) ? 0 : 1;
	}
}