########################################
# Test config file for AlpgenInterface #
########################################

import FWCore.ParameterSet.Config as cms

process = cms.Process("TEST")

###########################
# Basic process controls. #
###########################

process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(1)
    )

process.load("FWCore.MessageService.MessageLogger_cfi")

##########
# Source #
##########

process.source = cms.Source("PoolSource",
                            fileNames = cms.untracked.vstring('file:w2j.root')
                            )

process.analyzer = cms.EDAnalyzer("AlpgenExtractor",
                                  unwParFile = cms.untracked.string('w2j_unw.par'),
                                  wgtFile = cms.untracked.string('w2j.wgt'),
                                  parFile = cms.untracked.string('w2j.par')
                                  )

process.p = cms.Path(process.analyzer)
