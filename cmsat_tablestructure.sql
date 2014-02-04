-- MySQL dump 10.13  Distrib 5.5.34, for debian-linux-gnu (x86_64)
--
-- Host: localhost    Database: cmsat
-- ------------------------------------------------------
-- Server version	5.5.34-0ubuntu0.13.04.1

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Table structure for table `clauseGlueDistrib`
--

DROP TABLE IF EXISTS `clauseGlueDistrib`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `clauseGlueDistrib` (
  `runID` bigint(20) unsigned NOT NULL,
  `conflicts` int(20) unsigned NOT NULL,
  `glue` int(10) unsigned NOT NULL,
  `num` int(20) unsigned NOT NULL,
  KEY `idx1` (`runID`,`conflicts`,`glue`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `clauseSizeDistrib`
--

DROP TABLE IF EXISTS `clauseSizeDistrib`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `clauseSizeDistrib` (
  `runID` bigint(20) unsigned NOT NULL,
  `conflicts` int(20) unsigned NOT NULL,
  `size` int(10) unsigned NOT NULL,
  `num` int(20) unsigned NOT NULL,
  KEY `idx1` (`runID`,`conflicts`,`size`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `clauseStats`
--

DROP TABLE IF EXISTS `clauseStats`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `clauseStats` (
  `runID` bigint(20) unsigned NOT NULL,
  `simplifications` int(20) unsigned NOT NULL,
  `reduceDB` int(20) unsigned NOT NULL,
  `learnt` int(10) unsigned NOT NULL,
  `size` int(20) unsigned NOT NULL,
  `glue` int(20) unsigned NOT NULL,
  `numPropAndConfl` int(20) unsigned NOT NULL,
  `numLitVisited` int(20) unsigned NOT NULL,
  `numLookedAt` int(20) unsigned NOT NULL
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `fileNamesUsed`
--

DROP TABLE IF EXISTS `tags`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `tags` (
  `runID` bigint(20) NOT NULL,
  `tagname` varchar(500) NOT NULL,
  `tag` varchar(500) NOT NULL,
  KEY `idx1` (`runID`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `reduceDB`
--

DROP TABLE IF EXISTS `reduceDB`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `reduceDB` (
  `runID` bigint(20) unsigned NOT NULL,
  `simplifications` int(20) unsigned NOT NULL,
  `restarts` int(20) unsigned NOT NULL,
  `conflicts` int(20) unsigned NOT NULL,
  `time` float unsigned NOT NULL,
  `reduceDBs` int(20) unsigned NOT NULL,
  `irredClsVisited` int(20) unsigned NOT NULL,
  `irredLitsVisited` int(20) unsigned NOT NULL,
  `redClsVisited` int(20) unsigned NOT NULL,
  `redLitsVisited` int(20) unsigned NOT NULL,
  `preRemovedNum` int(20) unsigned NOT NULL,
  `preRemovedLits` int(20) unsigned NOT NULL,
  `preRemovedGlue` int(20) unsigned NOT NULL,
  `preRemovedResolBin` int(20) unsigned NOT NULL,
  `preRemovedResolTri` int(20) unsigned NOT NULL,
  `preRemovedResolLIrred` int(20) unsigned NOT NULL,
  `preRemovedResolLRed` int(20) unsigned NOT NULL,
  `preRemovedAge` int(20) unsigned NOT NULL,
  `preRemovedAct` float unsigned NOT NULL,
  `preRemovedLitVisited` int(20) unsigned NOT NULL,
  `preRemovedProp` int(20) unsigned NOT NULL,
  `preRemovedConfl` int(20) unsigned NOT NULL,
  `preRemovedLookedAt` int(20) unsigned NOT NULL,
  `preRemovedUsedUIP` int(20) unsigned NOT NULL,
  `removedNum` int(20) unsigned NOT NULL,
  `removedLits` int(20) unsigned NOT NULL,
  `removedGlue` int(20) unsigned NOT NULL,
  `removedResolBin` int(20) unsigned NOT NULL,
  `removedResolTri` int(20) unsigned NOT NULL,
  `removedResolLIrred` int(20) unsigned NOT NULL,
  `removedResolLRed` int(20) unsigned NOT NULL,
  `removedAge` int(20) unsigned NOT NULL,
  `removedAct` float unsigned NOT NULL,
  `removedLitVisited` int(20) unsigned NOT NULL,
  `removedProp` int(20) unsigned NOT NULL,
  `removedConfl` int(20) unsigned NOT NULL,
  `removedLookedAt` int(20) unsigned NOT NULL,
  `removedUsedUIP` int(20) unsigned NOT NULL,
  `remainNum` int(20) unsigned NOT NULL,
  `remainLits` int(20) unsigned NOT NULL,
  `remainGlue` int(20) unsigned NOT NULL,
  `remainResolBin` int(20) unsigned NOT NULL,
  `remainResolTri` int(20) unsigned NOT NULL,
  `remainResolLIrred` int(20) unsigned NOT NULL,
  `remainResolLRed` int(20) unsigned NOT NULL,
  `remainAge` int(20) unsigned NOT NULL,
  `remainAct` float unsigned NOT NULL,
  `remainLitVisited` int(20) unsigned NOT NULL,
  `remainProp` int(20) unsigned NOT NULL,
  `remainConfl` int(20) unsigned NOT NULL,
  `remainLookedAt` int(20) unsigned NOT NULL,
  `remainUsedUIP` int(20) unsigned NOT NULL,
  KEY `idx1` (`runID`,`conflicts`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `restart`
--

DROP TABLE IF EXISTS `restart`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `restart` (
  `runID` bigint(20) unsigned NOT NULL,
  `simplifications` int(20) unsigned NOT NULL,
  `restarts` int(20) unsigned NOT NULL,
  `conflicts` int(20) unsigned NOT NULL,
  `time` float unsigned NOT NULL,
  `numIrredBins` int(20) unsigned NOT NULL,
  `numIrredTris` int(20) unsigned NOT NULL,
  `numIrredLongs` int(20) unsigned NOT NULL,
  `numRedBins` int(20) unsigned NOT NULL,
  `numRedTris` int(20) unsigned NOT NULL,
  `numRedLongs` int(20) unsigned NOT NULL,
  `numIrredLits` int(20) unsigned NOT NULL,
  `numredLits` int(20) unsigned NOT NULL,
  `glue` float unsigned NOT NULL,
  `glueSD` float unsigned NOT NULL,
  `glueMin` int(20) unsigned NOT NULL,
  `glueMax` int(20) unsigned NOT NULL,
  `size` float unsigned NOT NULL,
  `sizeSD` float unsigned NOT NULL,
  `sizeMin` int(20) unsigned NOT NULL,
  `sizeMax` int(20) unsigned NOT NULL,
  `resolutions` float unsigned NOT NULL,
  `resolutionsSD` float unsigned NOT NULL,
  `resolutionsMin` int(20) unsigned NOT NULL,
  `resolutionsMax` int(20) unsigned NOT NULL,
  `conflAfterConfl` float unsigned NOT NULL,
  `branchDepth` float unsigned NOT NULL,
  `branchDepthSD` float unsigned NOT NULL,
  `branchDepthMin` int(20) unsigned NOT NULL,
  `branchDepthMax` int(20) unsigned NOT NULL,
  `branchDepthDelta` float unsigned NOT NULL,
  `branchDepthDeltaSD` float unsigned NOT NULL,
  `branchDepthDeltaMin` int(20) unsigned NOT NULL,
  `branchDepthDeltaMax` int(20) unsigned NOT NULL,
  `trailDepth` float unsigned NOT NULL,
  `trailDepthSD` float unsigned NOT NULL,
  `trailDepthMin` int(20) unsigned NOT NULL,
  `trailDepthMax` int(20) unsigned NOT NULL,
  `trailDepthDelta` float unsigned NOT NULL,
  `trailDepthDeltaSD` float unsigned NOT NULL,
  `trailDepthDeltaMin` int(20) unsigned NOT NULL,
  `trailDepthDeltaMax` int(20) unsigned NOT NULL,
  `agility` float unsigned NOT NULL,
  `propBinIrred` int(20) unsigned NOT NULL,
  `propBinRed` int(20) unsigned NOT NULL,
  `propTriIrred` int(20) unsigned NOT NULL,
  `propTriRed` int(20) unsigned NOT NULL,
  `propLongIrred` int(20) unsigned NOT NULL,
  `propLongRed` int(20) unsigned NOT NULL,
  `conflBinIrred` int(20) unsigned NOT NULL,
  `conflBinRed` int(20) unsigned NOT NULL,
  `conflTriIrred` int(20) unsigned NOT NULL,
  `conflTriRed` int(20) unsigned NOT NULL,
  `conflLongIrred` int(20) unsigned NOT NULL,
  `conflLongRed` int(20) unsigned NOT NULL,
  `learntUnits` int(20) unsigned NOT NULL,
  `learntBins` int(20) unsigned NOT NULL,
  `learntTris` int(20) unsigned NOT NULL,
  `learntLongs` int(20) unsigned NOT NULL,
  `watchListSizeTraversed` float unsigned NOT NULL,
  `watchListSizeTraversedSD` float unsigned NOT NULL,
  `watchListSizeTraversedMin` int(20) unsigned NOT NULL,
  `watchListSizeTraversedMax` int(20) unsigned NOT NULL,
  `resolBin` int(20) unsigned NOT NULL,
  `resolTri` int(20) unsigned NOT NULL,
  `resolLIrred` int(20) unsigned NOT NULL,
  `resolLRed` int(20) unsigned NOT NULL,
  `propagations` int(20) unsigned NOT NULL,
  `decisions` int(20) unsigned NOT NULL,
  `avgDecLevelVarLT` float unsigned NOT NULL,
  `avgTrailLevelVarLT` float unsigned NOT NULL,
  `avgDecLevelVar` float unsigned NOT NULL,
  `avgTrailLevelVar` float unsigned NOT NULL,
  `flipped` int(20) unsigned NOT NULL,
  `varSetPos` int(20) unsigned NOT NULL,
  `varSetNeg` int(20) unsigned NOT NULL,
  `free` int(20) unsigned NOT NULL,
  `replaced` int(20) unsigned NOT NULL,
  `eliminated` int(20) unsigned NOT NULL,
  `set` int(20) unsigned NOT NULL,
  KEY `idx1` (`runID`,`conflicts`),
  KEY `idx2` (`runID`,`simplifications`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `sizeGlue`
--

DROP TABLE IF EXISTS `sizeGlue`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `sizeGlue` (
  `runID` bigint(20) unsigned NOT NULL,
  `conflicts` int(20) unsigned NOT NULL,
  `size` int(10) unsigned NOT NULL,
  `glue` int(10) unsigned NOT NULL,
  `num` int(20) unsigned NOT NULL,
  KEY `idx1` (`runID`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `solverRun`
--

DROP TABLE IF EXISTS `solverRun`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `solverRun` (
  `runID` bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  `version` varchar(255) NOT NULL,
  `time` bigint(20) unsigned NOT NULL,
  PRIMARY KEY (`runID`)
) ENGINE=InnoDB AUTO_INCREMENT=15048711 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `startup`
--

DROP TABLE IF EXISTS `startup`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `startup` (
  `runID` bigint(20) unsigned NOT NULL,
  `startTime` datetime NOT NULL,
  `verbosity` int(20) unsigned NOT NULL,
  KEY `idx1` (`runID`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

DROP TABLE IF EXISTS `finishup`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `finishup` (
  `runID` bigint(20) unsigned NOT NULL,
  `endTime` datetime NOT NULL,
  `status` varchar(255) NOT NULL,
  KEY `idx1` (`runID`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `sum_clause_stats`
--

DROP TABLE IF EXISTS `sum_clause_stats`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `sum_clause_stats` (
  `runID` bigint(20) unsigned NOT NULL,
  `simplifications` int(20) unsigned NOT NULL,
  `reduceDB` int(20) unsigned NOT NULL,
  `learnt` int(10) unsigned NOT NULL,
  `avg_size` double unsigned NOT NULL,
  `avg_glue` double unsigned NOT NULL,
  `avg_props` double unsigned NOT NULL,
  `avg_confls` double unsigned NOT NULL,
  `avg_UIP_used` double unsigned NOT NULL,
  `avg_numPropAndConfl` int(20) unsigned NOT NULL,
  `avg_numLitVisited` int(20) unsigned NOT NULL,
  `avg_numLookedAt` int(20) unsigned NOT NULL,
  `num` int(20) unsigned NOT NULL,
  KEY `idx1` (`runID`,`reduceDB`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `varDataInit`
--

DROP TABLE IF EXISTS `varDataInit`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `varDataInit` (
  `varInitID` bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  `runID` bigint(20) unsigned NOT NULL,
  `simplifications` int(20) unsigned NOT NULL,
  `restarts` int(20) unsigned NOT NULL,
  `conflicts` int(20) unsigned NOT NULL,
  `time` float unsigned NOT NULL,
  PRIMARY KEY (`varInitID`),
  KEY `runID` (`runID`,`varInitID`)
) ENGINE=MyISAM AUTO_INCREMENT=15587 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `vars`
--

DROP TABLE IF EXISTS `vars`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `vars` (
  `varInitID` bigint(20) unsigned NOT NULL,
  `var` int(20) unsigned NOT NULL,
  `posPolarSet` int(20) unsigned NOT NULL,
  `negPolarSet` int(20) unsigned NOT NULL,
  `flippedPolarity` int(20) unsigned NOT NULL,
  `posDecided` int(20) unsigned NOT NULL,
  `negDecided` int(20) unsigned NOT NULL,
  `decLevelAvg` float NOT NULL,
  `decLevelSD` float NOT NULL,
  `decLevelMin` int(20) unsigned NOT NULL,
  `decLevelMax` int(20) unsigned NOT NULL,
  `trailLevelAvg` float NOT NULL,
  `trailLevelSD` float NOT NULL,
  `trailLevelMin` int(20) unsigned NOT NULL,
  `trailLevelMax` int(20) unsigned NOT NULL,
  KEY `varInitID` (`varInitID`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2014-01-15 17:59:44
