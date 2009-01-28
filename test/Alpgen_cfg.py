########################################
# Test config file for AlpgenInterface #
########################################

import FWCore.ParameterSet.Config as cms

process = cms.Process("PROD")

###########################
# Basic process controls. #
###########################

# Number of events. If you ask for more events than the number available in the .unw file,
# the AlpgenSource will quit after processing the last event in that file (i.e., will quit
# early). Also note that the number of events "available" in the .unw file is NOT the total
# number of events in that file - the matching efficiency has to be taken into account.
# please check the documentation.
process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(9999)
    )

# MessageLogger control. The standard MessageLogger configuration messes up the output,
# because things are written to STDOUT from different places. The configuration below
# fixes that, but gives a rather terse output. Configure as needed. 
process.MessageLogger = cms.Service("MessageLogger",
                                    destinations = cms.untracked.vstring("cout"),
                                    #categories = cms.untracked.vstring("FwkJob"),
                                    cout = cms.untracked.PSet(default = cms.untracked.PSet(limit = cms.untracked.int32(0))
                                                              #FwkJob = cms.untracked.PSet(limit = cms.untracked.int32(1))
                                                              )
                                    #fwkJobReports = cms.untracked.vstring("FrameworkJobReport.xml")
                                    )
# In case you want the default MessageLogger, use this.
#process.load("FWCore.MessageService.MessageLogger_cfi")

# Random Number Generator Service
process.RandomNumberGeneratorService = cms.Service("RandomNumberGeneratorService",
                                                   theSource = cms.PSet(initialSeed = cms.untracked.uint32(98765))
                                                   )
################
# AlpgenSource #
################

# The source file - here you put the unweighted file name without the .unw suffix.
# BUT, don't forget the file: prefix.
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

# The output of a source is the HepMCProduct. If you want the more standard genParticles, uncomment the lines below.
#process.load("SimGeneral.HepPDTESSource.pythiapdt_cfi")
#process.load("PhysicsTools.HepMCCandAlgos.genParticles_cfi")
#process.p1 = cms.Path(process.genParticles)

##########
# Output #
##########

process.GEN = cms.OutputModule("PoolOutputModule",
                               fileName = cms.untracked.string("alpgen.root"),
                               )

process.e = cms.EndPath(process.GEN)
