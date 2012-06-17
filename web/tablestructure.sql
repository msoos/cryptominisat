-- MySQL dump 10.13  Distrib 5.1.58, for debian-linux-gnu (x86_64)
--
-- Host: localhost    Database: cryptoms
-- ------------------------------------------------------
-- Server version	5.1.58-1

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
-- Table structure for table `clauseSizeDistrib`
--

DROP TABLE IF EXISTS `clauseSizeDistrib`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `clauseSizeDistrib` (
  `runID` bigint(20) unsigned NOT NULL,
  `conflicts` bigint(20) unsigned NOT NULL,
  `size` int(10) unsigned NOT NULL,
  `num` bigint(20) unsigned NOT NULL
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
  `simplifications` bigint(20) unsigned NOT NULL,
  `reduceDB` bigint(20) unsigned NOT NULL,
  `learnt` int(10) unsigned NOT NULL,
  `size` bigint(20) unsigned NOT NULL,
  `glue` bigint(20) unsigned NOT NULL,
  `numPropAndConfl` bigint(20) unsigned NOT NULL,
  `numLitVisited` bigint(20) unsigned NOT NULL,
  `numLookedAt` bigint(20) unsigned NOT NULL
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `confls`
--

DROP TABLE IF EXISTS `confls`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `confls` (
  `runID` bigint(20) unsigned NOT NULL,
  `simplifications` bigint(20) unsigned NOT NULL,
  `binIrred` bigint(20) unsigned NOT NULL,
  `binRed` bigint(20) unsigned NOT NULL,
  `tri` bigint(20) unsigned NOT NULL,
  `longIrred` bigint(20) unsigned NOT NULL,
  `longRed` bigint(20) unsigned NOT NULL
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `learnts`
--

DROP TABLE IF EXISTS `learnts`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `learnts` (
  `runID` bigint(20) unsigned NOT NULL,
  `simplifications` bigint(20) unsigned NOT NULL,
  `units` bigint(20) unsigned NOT NULL,
  `bins` bigint(20) unsigned NOT NULL,
  `tris` bigint(20) unsigned NOT NULL,
  `longs` bigint(20) unsigned NOT NULL
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `polarSet`
--

DROP TABLE IF EXISTS `polarSet`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `polarSet` (
  `runID` bigint(20) unsigned NOT NULL,
  `simplifications` bigint(20) unsigned NOT NULL,
  `order` int(10) unsigned NOT NULL,
  `pos` bigint(20) unsigned NOT NULL,
  `neg` bigint(20) unsigned NOT NULL,
  `total` bigint(20) unsigned NOT NULL,
  `flipped` bigint(20) unsigned NOT NULL
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `props`
--

DROP TABLE IF EXISTS `props`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `props` (
  `runID` bigint(20) unsigned NOT NULL,
  `simplifications` bigint(20) unsigned NOT NULL,
  `unit` bigint(20) unsigned NOT NULL,
  `binIrred` bigint(20) unsigned NOT NULL,
  `binRed` bigint(20) unsigned NOT NULL,
  `tri` bigint(20) unsigned NOT NULL,
  `longIrred` bigint(20) unsigned NOT NULL,
  `longRed` bigint(20) unsigned NOT NULL
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
  `simplifications` bigint(20) unsigned NOT NULL,
  `restarts` bigint(20) unsigned NOT NULL,
  `conflicts` bigint(20) unsigned NOT NULL,
  `time` double unsigned NOT NULL,
  `glue` double unsigned NOT NULL,
  `size` double unsigned NOT NULL,
  `resolutions` double unsigned NOT NULL,
  `branchDepth` double unsigned NOT NULL,
  `branchDepthDelta` double unsigned NOT NULL,
  `trailDepth` double unsigned NOT NULL,
  `trailDepthDelta` double unsigned NOT NULL,
  `agility` double unsigned NOT NULL,
  `glueSD` double unsigned NOT NULL,
  `sizeSD` double unsigned NOT NULL,
  `resolutionsSD` double unsigned NOT NULL,
  `branchDepthSD` double unsigned NOT NULL,
  `branchDepthDeltaSD` double unsigned NOT NULL,
  `trailDepthSD` double unsigned NOT NULL,
  `trailDepthDeltaSD` double unsigned NOT NULL,
  `agilitySD` double unsigned NOT NULL,
  `propBinIrred` bigint(20) unsigned NOT NULL,
  `propBinRed` bigint(20) unsigned NOT NULL,
  `propTri` bigint(20) unsigned NOT NULL,
  `propLongIrred` bigint(20) unsigned NOT NULL,
  `propLongRed` bigint(20) unsigned NOT NULL,
  `conflBinIrred` bigint(20) unsigned NOT NULL,
  `conflBinRed` bigint(20) unsigned NOT NULL,
  `conflTri` bigint(20) unsigned NOT NULL,
  `conflLongIrred` bigint(20) unsigned NOT NULL,
  `conflLongRed` bigint(20) unsigned NOT NULL,
  `learntUnits` bigint(20) unsigned NOT NULL,
  `learntBins` bigint(20) unsigned NOT NULL,
  `learntTris` bigint(20) unsigned NOT NULL,
  `learntLongs` bigint(20) unsigned NOT NULL,
  `propsPerDec` double unsigned NOT NULL,
  `flippedPercent` double unsigned NOT NULL,
  `varSetPos` bigint(20) unsigned NOT NULL,
  `varSetNeg` bigint(20) unsigned NOT NULL,
  `free` bigint(20) unsigned NOT NULL,
  `replaced` bigint(20) unsigned NOT NULL,
  `eliminated` bigint(20) unsigned NOT NULL,
  `set` bigint(20) unsigned NOT NULL
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `vars`
--

DROP TABLE IF EXISTS `vars`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `vars` (
  `runID` bigint(20) unsigned NOT NULL,
  `conflicts` bigint(20) unsigned NOT NULL,
  `free` bigint(20) unsigned NOT NULL,
  `replaced` bigint(20) unsigned NOT NULL,
  `eliminated` bigint(20) unsigned NOT NULL,
  `set` bigint(20) unsigned NOT NULL
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

-- Dump completed on 2012-06-17 14:26:26
