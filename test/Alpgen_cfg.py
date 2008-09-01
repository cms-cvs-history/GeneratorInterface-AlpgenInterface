########################################
# Test config file for AlpgenInterface #
########################################

import FWCore.ParameterSet.Config as cms

process = cms.Process("PROD")

###########################
# Basic process controls. #
###########################

# Number of events.
process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(100)
    )

# MessageLogger control. Uncomment lines and fix commas to receive messages from FwkJob.
process.MessageLogger = cms.Service("MessageLogger",
                                    destinations = cms.untracked.vstring("cout"),
                                    #categories = cms.untracked.vstring("FwkJob"),
                                    cout = cms.untracked.PSet(default = cms.untracked.PSet(limit = cms.untracked.int32(0))
                                                              #FwkJob = cms.untracked.PSet(limit = cms.untracked.int32(1))
                                                              )
                                    #fwkJobReports = cms.untracked.vstring("FrameworkJobReport.xml")
                                    )

# Random Number Generator Service
process.RandomNumberGeneratorService = cms.Service("RandomNumberGeneratorService",
                                                   theSource = cms.PSet(initialSeed = cms.untracked.uint32(98765))
                                                   )
################
# AlpgenSource #
################

# The source file - here you put the unweighted file name without the .unw suffix.                                                   
process.source = cms.Source("AlpgenSource",
                            fileNames = cms.untracked.vstring('file:alpgen'),
                            pythiaPylistVerbosity = cms.untracked.int32(0),
                            pythiaHepMCVerbosity = cms.untracked.bool(False),
                            PythiaParameters = cms.PSet(parameterSets = cms.vstring("pythia"),
                                                        pythia = cms.vstring("MSEL=0              !(D=1) ",
                                                                             "MSTJ(11)=3          !Choice of the fragmentation function",
                                                                             "MSTP(143)=1         !Call the matching routine in ALPGEN"
                                                                             )
                                                        ),
                            GeneratorParameters = cms.PSet(parameterSets = cms.vstring("generator"),
                                                           generator = cms.vstring("IXpar(2) = 1   ! inclus./exclus. sample: 0/1",
                                                                                   #Inputs for clustering: minET(CLUS), deltaR(CLUS)
                                                                                   "RXpar(1) = 20. ! ETCLUS : minET(CLUS)",
                                                                                   "RXpar(2) = 0.7 ! RCLUS  : deltaR(CLUS)"
                                                                                   )
                                                           )
                            )

# This filters out empty (rejected by matching) events from the PoolOutputModule.
process.filter = cms.EDFilter("AlpgenEmptyEventFilter")

process.p1 = cms.Path(process.filter)

##########
# Output #
##########

process.GEN = cms.OutputModule("PoolOutputModule",
                               fileName = cms.untracked.string("alpgen.root"),
                               SelectEvents = cms.untracked.PSet(SelectEvents = cms.vstring("p1"))
                               )

process.e = cms.EndPath(process.GEN)
