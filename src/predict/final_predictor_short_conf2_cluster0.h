/******************************************
Copyright (c) 2018, Mate Soos

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
***********************************************/

#include "clause.h"
#include "reducedb.h"

namespace CMSat {

static double estimator_should_keep_short_conf2_cluster0_0(
    const CMSat::Clause* cl
    , const uint64_t sumConflicts
    , const uint32_t rdb0_last_touched_diff
    , const uint32_t rdb0_act_ranking
    , const uint32_t rdb0_act_ranking_top_10
) {

    double rdb_rel_used_for_uip_creation = 0;
    if (cl->stats.used_for_uip_creation > cl->stats.rdb1_used_for_uip_creation) {
        rdb_rel_used_for_uip_creation = 1.0;
    } else {
        rdb_rel_used_for_uip_creation = 0.0;
    }

    double rdb0_avg_confl;
    if (cl->stats.sum_delta_confl_uip1_used == 0) {
        rdb0_avg_confl = 0;
    } else {
        rdb0_avg_confl = ((double)cl->stats.sum_uip1_used)/((double)cl->stats.sum_delta_confl_uip1_used);
    }

    double rdb0_used_per_confl;
    if (sumConflicts-cl->stats.introduced_at_conflict == 0) {
        rdb0_used_per_confl = 0;
    } else {
        rdb0_used_per_confl = ((double)cl->stats.sum_uip1_used)/((double)sumConflicts-(double)cl->stats.introduced_at_conflict);
    }
    if ( rdb0_act_ranking_top_10 <= 2.5f ) {
        if ( cl->stats.antec_num_total_lits_rel <= 0.475569367409f ) {
            if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                if ( cl->size() <= 10.5f ) {
                    if ( cl->stats.rdb1_last_touched_diff <= 18403.5f ) {
                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                            if ( rdb0_last_touched_diff <= 13848.5f ) {
                                if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                        return 858.0/3235.3;
                                    } else {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 9.5f ) {
                                            if ( cl->stats.num_overlap_literals <= 6.5f ) {
                                                return 271.0/1823.2;
                                            } else {
                                                return 292.0/1585.1;
                                            }
                                        } else {
                                            return 251.0/2378.7;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0272082053125f ) {
                                            return 327.0/1499.6;
                                        } else {
                                            if ( cl->stats.size_rel <= 0.299079269171f ) {
                                                return 528.0/2732.7;
                                            } else {
                                                return 345.0/2370.5;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.size_rel <= 0.196774721146f ) {
                                            return 368.0/2637.1;
                                        } else {
                                            return 297.0/3194.6;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 12.5f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0253330431879f ) {
                                        return 398.0/1644.1;
                                    } else {
                                        return 303.0/1644.1;
                                    }
                                } else {
                                    if ( cl->size() <= 7.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0969649404287f ) {
                                            return 582.0/2238.3;
                                        } else {
                                            return 513.0/2394.9;
                                        }
                                    } else {
                                        if ( cl->stats.size_rel <= 0.384086072445f ) {
                                            return 802.0/2372.6;
                                        } else {
                                            return 359.0/1574.9;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 20488.0f ) {
                                if ( cl->size() <= 7.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.0676505863667f ) {
                                        return 377.0/1367.4;
                                    } else {
                                        if ( rdb0_last_touched_diff <= 11635.5f ) {
                                            return 307.0/2018.5;
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.153420269489f ) {
                                                return 308.0/1481.3;
                                            } else {
                                                return 448.0/1878.1;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.596588432789f ) {
                                        return 494.0/1597.3;
                                    } else {
                                        return 355.0/1601.4;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.164588034153f ) {
                                    return 706.0/2081.6;
                                } else {
                                    return 507.0/1227.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 73359.5f ) {
                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 12.5f ) {
                                        return 377.0/1349.1;
                                    } else {
                                        if ( cl->stats.size_rel <= 0.135384380817f ) {
                                            return 424.0/1353.1;
                                        } else {
                                            if ( cl->stats.glue <= 5.5f ) {
                                                return 558.0/1070.3;
                                            } else {
                                                return 552.0/1371.4;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 13.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0273979026824f ) {
                                            return 538.0/1074.4;
                                        } else {
                                            return 449.0/1395.9;
                                        }
                                    } else {
                                        if ( cl->stats.dump_number <= 12.5f ) {
                                            return 971.0/2169.1;
                                        } else {
                                            if ( rdb0_last_touched_diff <= 43858.5f ) {
                                                return 676.0/1406.0;
                                            } else {
                                                return 852.0/1308.4;
                                            }
                                        }
                                    }
                                }
                            } else {
                                return 908.0/1269.7;
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals_rel <= 0.0365998037159f ) {
                                return 429.0/1263.6;
                            } else {
                                if ( cl->stats.num_overlap_literals <= 7.5f ) {
                                    return 381.0/1507.8;
                                } else {
                                    return 530.0/1674.6;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                        if ( rdb0_last_touched_diff <= 48529.5f ) {
                            if ( cl->stats.dump_number <= 4.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 91.5f ) {
                                    if ( rdb0_last_touched_diff <= 10810.0f ) {
                                        return 494.0/1387.7;
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.959582805634f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 17167.0f ) {
                                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                                    return 810.0/1611.5;
                                                } else {
                                                    return 623.0/854.6;
                                                }
                                            } else {
                                                if ( cl->stats.num_antecedents_rel <= 0.168197602034f ) {
                                                    return 629.0/883.1;
                                                } else {
                                                    return 702.0/807.8;
                                                }
                                            }
                                        } else {
                                            return 906.0/700.0;
                                        }
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 22672.5f ) {
                                        return 707.0/677.6;
                                    } else {
                                        return 818.0/386.6;
                                    }
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                    if ( cl->size() <= 48.5f ) {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.579861104488f ) {
                                            if ( cl->stats.glue <= 6.5f ) {
                                                return 624.0/1813.0;
                                            } else {
                                                if ( cl->stats.glue_rel_long <= 0.625698328018f ) {
                                                    return 606.0/1568.8;
                                                } else {
                                                    return 819.0/1813.0;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.204012066126f ) {
                                                if ( rdb0_last_touched_diff <= 11144.5f ) {
                                                    return 421.0/1224.9;
                                                } else {
                                                    return 748.0/1766.2;
                                                }
                                            } else {
                                                return 1069.0/1884.2;
                                            }
                                        }
                                    } else {
                                        return 735.0/1151.7;
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.0933368578553f ) {
                                        return 729.0/868.9;
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 30.5f ) {
                                            return 582.0/1224.9;
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 15250.0f ) {
                                                return 638.0/1308.4;
                                            } else {
                                                return 1186.0/1524.1;
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.glue <= 13.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.133761763573f ) {
                                    if ( rdb0_last_touched_diff <= 68256.5f ) {
                                        return 637.0/893.3;
                                    } else {
                                        return 941.0/807.8;
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 84563.5f ) {
                                        if ( cl->stats.dump_number <= 7.5f ) {
                                            return 871.0/563.6;
                                        } else {
                                            return 1046.0/1214.8;
                                        }
                                    } else {
                                        return 1121.0/622.6;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 89.5f ) {
                                    return 810.0/455.8;
                                } else {
                                    return 939.0/372.4;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 4.5f ) {
                            if ( cl->stats.glue_rel_long <= 0.816421866417f ) {
                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                    if ( cl->size() <= 22.5f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.167120233178f ) {
                                            return 376.0/1534.2;
                                        } else {
                                            return 421.0/1326.7;
                                        }
                                    } else {
                                        return 496.0/1145.6;
                                    }
                                } else {
                                    return 724.0/1434.5;
                                }
                            } else {
                                return 890.0/1959.5;
                            }
                        } else {
                            if ( cl->stats.dump_number <= 2.5f ) {
                                return 654.0/2256.6;
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 2033.5f ) {
                                    return 379.0/2608.6;
                                } else {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 8.5f ) {
                                        return 435.0/1764.2;
                                    } else {
                                        return 345.0/2158.9;
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                if ( rdb0_last_touched_diff <= 3419.5f ) {
                    if ( cl->stats.used_for_uip_creation <= 7.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 10005.5f ) {
                            if ( cl->stats.num_overlap_literals <= 42.5f ) {
                                if ( rdb0_last_touched_diff <= 1002.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.0935423150659f ) {
                                        if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                            return 211.0/2293.2;
                                        } else {
                                            return 142.0/2340.0;
                                        }
                                    } else {
                                        if ( cl->stats.glue <= 6.5f ) {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.1470964849f ) {
                                                if ( cl->stats.num_antecedents_rel <= 0.147378861904f ) {
                                                    return 54.0/2038.9;
                                                } else {
                                                    return 85.0/2203.7;
                                                }
                                            } else {
                                                return 106.0/2232.2;
                                            }
                                        } else {
                                            return 192.0/3265.8;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 28.5f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 12.5f ) {
                                            if ( cl->stats.glue_rel_queue <= 0.301396071911f ) {
                                                return 121.0/1843.5;
                                            } else {
                                                return 192.0/3414.4;
                                            }
                                        } else {
                                            if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                                if ( cl->stats.size_rel <= 0.231210589409f ) {
                                                    return 234.0/1808.9;
                                                } else {
                                                    return 234.0/2775.4;
                                                }
                                            } else {
                                                return 186.0/3560.9;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.dump_number <= 11.5f ) {
                                            return 361.0/2997.2;
                                        } else {
                                            return 271.0/2818.2;
                                        }
                                    }
                                }
                            } else {
                                return 272.0/1998.2;
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.690688788891f ) {
                                if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                    if ( cl->stats.size_rel <= 0.247117623687f ) {
                                        return 428.0/2173.1;
                                    } else {
                                        return 439.0/3107.1;
                                    }
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0709571689367f ) {
                                        return 234.0/1642.1;
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 7.5f ) {
                                            return 166.0/2142.6;
                                        } else {
                                            return 226.0/2173.1;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.692971944809f ) {
                                    return 303.0/1796.7;
                                } else {
                                    return 306.0/1491.5;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 6.5f ) {
                            if ( rdb0_last_touched_diff <= 779.5f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.0500814989209f ) {
                                    if ( cl->stats.size_rel <= 0.0845240056515f ) {
                                        return 89.0/1971.7;
                                    } else {
                                        return 128.0/1849.6;
                                    }
                                } else {
                                    if ( cl->size() <= 11.5f ) {
                                        if ( cl->stats.num_overlap_literals <= 4.5f ) {
                                            return 42.0/3005.4;
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.155065476894f ) {
                                                return 70.0/2006.3;
                                            } else {
                                                return 89.0/3733.8;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.used_for_uip_creation <= 15.5f ) {
                                            return 149.0/2517.0;
                                        } else {
                                            return 74.0/3939.3;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 11.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 10699.0f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 23.5f ) {
                                            return 120.0/2460.1;
                                        } else {
                                            return 159.0/1833.3;
                                        }
                                    } else {
                                        return 182.0/1668.5;
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.0515906140208f ) {
                                        return 134.0/1882.2;
                                    } else {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                            if ( cl->stats.num_overlap_literals <= 10.5f ) {
                                                return 172.0/2584.2;
                                            } else {
                                                return 96.0/2053.1;
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 0.4425329566f ) {
                                                return 90.0/2445.8;
                                            } else {
                                                return 77.0/2407.1;
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals_rel <= 0.0250705182552f ) {
                                if ( rdb0_last_touched_diff <= 792.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.155323833227f ) {
                                        return 99.0/2836.5;
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.285380482674f ) {
                                            if ( cl->stats.used_for_uip_creation <= 35.5f ) {
                                                return 56.0/2002.2;
                                            } else {
                                                return 55.0/2976.9;
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 29.5f ) {
                                                if ( cl->stats.num_antecedents_rel <= 0.0354732349515f ) {
                                                    return 64.0/1975.8;
                                                } else {
                                                    return 31.0/2191.5;
                                                }
                                            } else {
                                                if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                                    return 21.0/3037.9;
                                                } else {
                                                    return 34.0/2409.2;
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 15.5f ) {
                                        return 143.0/2690.0;
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.0387478172779f ) {
                                            return 139.0/3373.7;
                                        } else {
                                            return 79.0/3548.7;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 12.5f ) {
                                    if ( cl->stats.num_overlap_literals <= 12.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.525869727135f ) {
                                            if ( cl->stats.glue_rel_queue <= 0.363810539246f ) {
                                                return 51.0/2362.4;
                                            } else {
                                                return 21.0/2183.3;
                                            }
                                        } else {
                                            return 66.0/2213.8;
                                        }
                                    } else {
                                        return 158.0/3849.8;
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.0891969352961f ) {
                                        if ( cl->stats.glue <= 4.5f ) {
                                            return 75.0/3200.7;
                                        } else {
                                            return 101.0/3483.5;
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 1210.5f ) {
                                            if ( cl->stats.num_antecedents_rel <= 0.126444265246f ) {
                                                if ( cl->stats.num_antecedents_rel <= 0.11080263555f ) {
                                                    return 30.0/2604.5;
                                                } else {
                                                    return 35.0/2045.0;
                                                }
                                            } else {
                                                if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                                    if ( cl->stats.num_total_lits_antecedents <= 46.5f ) {
                                                        if ( cl->stats.num_overlap_literals <= 8.5f ) {
                                                            return 27.0/3044.0;
                                                        } else {
                                                            return 40.0/2244.4;
                                                        }
                                                    } else {
                                                        return 32.0/4195.7;
                                                    }
                                                } else {
                                                    if ( cl->stats.num_antecedents_rel <= 0.17366014421f ) {
                                                        return 6.0/2722.5;
                                                    } else {
                                                        if ( cl->stats.glue_rel_queue <= 0.464268058538f ) {
                                                            return 17.0/3141.7;
                                                        } else {
                                                            return 37.0/3648.4;
                                                        }
                                                    }
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.200689196587f ) {
                                                if ( rdb0_last_touched_diff <= 1960.0f ) {
                                                    return 46.0/2132.5;
                                                } else {
                                                    return 68.0/2024.6;
                                                }
                                            } else {
                                                return 69.0/3632.1;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_last_touched_diff <= 10840.0f ) {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                            if ( cl->stats.used_for_uip_creation <= 5.5f ) {
                                if ( cl->stats.glue_rel_queue <= 0.620518505573f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.468091756105f ) {
                                        if ( cl->size() <= 5.5f ) {
                                            return 271.0/1615.6;
                                        } else {
                                            return 421.0/2085.7;
                                        }
                                    } else {
                                        return 256.0/2114.1;
                                    }
                                } else {
                                    return 583.0/2818.2;
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.121554195881f ) {
                                    return 238.0/2506.9;
                                } else {
                                    return 285.0/2095.8;
                                }
                            }
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 10.5f ) {
                                if ( cl->stats.num_overlap_literals <= 9.5f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 10.5f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.0857158005238f ) {
                                            return 242.0/2081.6;
                                        } else {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.101053103805f ) {
                                                return 158.0/1737.7;
                                            } else {
                                                return 143.0/2425.5;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.dump_number <= 14.5f ) {
                                            return 142.0/2773.4;
                                        } else {
                                            return 142.0/2171.1;
                                        }
                                    }
                                } else {
                                    if ( cl->size() <= 9.5f ) {
                                        return 205.0/2641.1;
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 54.5f ) {
                                            return 281.0/2006.3;
                                        } else {
                                            return 234.0/2219.9;
                                        }
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 5341.0f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0322199240327f ) {
                                        return 128.0/2010.4;
                                    } else {
                                        return 129.0/3520.2;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 400.0f ) {
                                        return 175.0/1802.8;
                                    } else {
                                        return 157.0/2423.4;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_long <= 0.727715969086f ) {
                            if ( rdb0_last_touched_diff <= 4493.5f ) {
                                return 405.0/2539.4;
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 36839.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.504806160927f ) {
                                        return 623.0/2887.4;
                                    } else {
                                        return 270.0/1745.8;
                                    }
                                } else {
                                    return 477.0/1815.0;
                                }
                            }
                        } else {
                            return 720.0/2297.3;
                        }
                    }
                }
            }
        } else {
            if ( rdb0_last_touched_diff <= 9979.5f ) {
                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                    if ( cl->size() <= 7.5f ) {
                        return 144.0/2535.3;
                    } else {
                        if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                            if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                if ( cl->stats.dump_number <= 7.5f ) {
                                    return 326.0/1426.4;
                                } else {
                                    return 238.0/1678.7;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 182.5f ) {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                        return 327.0/1481.3;
                                    } else {
                                        return 704.0/1929.0;
                                    }
                                } else {
                                    return 492.0/1086.6;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 7618.0f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 1.00349283218f ) {
                                    if ( rdb0_last_touched_diff <= 1313.0f ) {
                                        return 81.0/3278.0;
                                    } else {
                                        return 130.0/2413.3;
                                    }
                                } else {
                                    return 123.0/1857.8;
                                }
                            } else {
                                return 283.0/2685.9;
                            }
                        }
                    }
                } else {
                    return 530.0/1774.3;
                }
            } else {
                if ( cl->stats.dump_number <= 12.5f ) {
                    if ( cl->stats.glue_rel_long <= 0.947625935078f ) {
                        if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                            if ( cl->stats.glue_rel_queue <= 0.807495236397f ) {
                                if ( cl->stats.size_rel <= 0.896616339684f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.642128229141f ) {
                                        return 408.0/1343.0;
                                    } else {
                                        return 661.0/1375.5;
                                    }
                                } else {
                                    return 672.0/864.8;
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 24127.0f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 7.59166669846f ) {
                                        return 642.0/1064.2;
                                    } else {
                                        return 642.0/767.1;
                                    }
                                } else {
                                    return 807.0/508.7;
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.75705575943f ) {
                                return 710.0/661.3;
                            } else {
                                return 896.0/427.3;
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 47963.5f ) {
                            if ( cl->stats.num_overlap_literals <= 442.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 7.72134017944f ) {
                                    return 1476.0/1182.2;
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                        return 1291.0/571.8;
                                    } else {
                                        return 1654.0/506.7;
                                    }
                                }
                            } else {
                                return 1561.0/284.9;
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 18.6450004578f ) {
                                return 1415.0/354.1;
                            } else {
                                return 1481.0/81.4;
                            }
                        }
                    }
                } else {
                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 13845.5f ) {
                            return 475.0/1351.1;
                        } else {
                            return 579.0/907.5;
                        }
                    } else {
                        if ( cl->stats.glue_rel_queue <= 0.950626850128f ) {
                            return 848.0/1304.3;
                        } else {
                            return 693.0/785.4;
                        }
                    }
                }
            }
        }
    } else {
        if ( cl->stats.antecedents_glue_long_reds_var <= 1.00080001354f ) {
            if ( cl->stats.rdb1_last_touched_diff <= 50553.0f ) {
                if ( cl->size() <= 10.5f ) {
                    if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 22719.5f ) {
                            if ( cl->stats.glue_rel_queue <= 0.619405269623f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0307722650468f ) {
                                    return 608.0/1802.8;
                                } else {
                                    if ( rdb0_last_touched_diff <= 18667.5f ) {
                                        return 434.0/2224.0;
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 14697.0f ) {
                                            return 477.0/1249.4;
                                        } else {
                                            return 497.0/1548.5;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.183540463448f ) {
                                    return 509.0/1322.6;
                                } else {
                                    return 491.0/1074.4;
                                }
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals <= 6.5f ) {
                                if ( cl->stats.dump_number <= 14.5f ) {
                                    return 933.0/2177.2;
                                } else {
                                    return 575.0/1009.3;
                                }
                            } else {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.173749998212f ) {
                                    if ( cl->stats.dump_number <= 5.5f ) {
                                        return 670.0/970.6;
                                    } else {
                                        return 798.0/1495.6;
                                    }
                                } else {
                                    return 645.0/860.7;
                                }
                            }
                        }
                    } else {
                        return 330.0/2244.4;
                    }
                } else {
                    if ( cl->stats.glue_rel_queue <= 0.685766577721f ) {
                        if ( cl->stats.size_rel <= 0.806308865547f ) {
                            if ( cl->stats.glue_rel_long <= 0.286295861006f ) {
                                return 518.0/1515.9;
                            } else {
                                if ( rdb0_last_touched_diff <= 34048.0f ) {
                                    if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                        return 656.0/1762.1;
                                    } else {
                                        return 781.0/1666.5;
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 6.5f ) {
                                        return 710.0/759.0;
                                    } else {
                                        return 667.0/1039.8;
                                    }
                                }
                            }
                        } else {
                            return 710.0/777.3;
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.00546081503853f ) {
                            if ( rdb0_last_touched_diff <= 17577.0f ) {
                                return 593.0/1408.1;
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.114286497235f ) {
                                    return 854.0/559.6;
                                } else {
                                    if ( cl->stats.dump_number <= 6.5f ) {
                                        if ( rdb0_last_touched_diff <= 36226.0f ) {
                                            return 688.0/744.7;
                                        } else {
                                            return 794.0/500.6;
                                        }
                                    } else {
                                        return 589.0/1080.5;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 62.5f ) {
                                return 959.0/868.9;
                            } else {
                                return 1473.0/761.0;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.size_rel <= 0.494619220495f ) {
                    if ( cl->stats.glue_rel_long <= 0.627167165279f ) {
                        if ( cl->stats.num_antecedents_rel <= 0.2107501477f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 149493.0f ) {
                                if ( cl->stats.num_overlap_literals <= 8.5f ) {
                                    if ( cl->stats.num_overlap_literals <= 5.5f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.126258701086f ) {
                                            if ( cl->stats.num_total_lits_antecedents <= 11.5f ) {
                                                return 697.0/1133.4;
                                            } else {
                                                return 699.0/978.7;
                                            }
                                        } else {
                                            return 838.0/1015.4;
                                        }
                                    } else {
                                        return 947.0/1497.6;
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 32.5f ) {
                                        return 841.0/944.1;
                                    } else {
                                        return 778.0/543.3;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 16.5f ) {
                                    if ( rdb0_act_ranking_top_10 <= 7.5f ) {
                                        return 1000.0/990.9;
                                    } else {
                                        return 924.0/667.4;
                                    }
                                } else {
                                    if ( cl->size() <= 8.5f ) {
                                        return 874.0/655.2;
                                    } else {
                                        if ( cl->stats.glue <= 7.5f ) {
                                            return 1036.0/360.2;
                                        } else {
                                            return 802.0/402.9;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.dump_number <= 15.5f ) {
                                return 1152.0/1271.7;
                            } else {
                                if ( rdb0_last_touched_diff <= 207390.0f ) {
                                    if ( rdb0_last_touched_diff <= 132234.5f ) {
                                        return 760.0/850.5;
                                    } else {
                                        return 950.0/756.9;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 13.5f ) {
                                        return 989.0/441.5;
                                    } else {
                                        return 828.0/480.2;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.num_overlap_literals <= 8.5f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 17.5f ) {
                                return 1078.0/1143.5;
                            } else {
                                return 998.0/602.3;
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 32.5f ) {
                                return 1292.0/966.5;
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                    return 945.0/537.2;
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.215305149555f ) {
                                        return 920.0/282.8;
                                    } else {
                                        return 926.0/427.3;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_last_touched_diff <= 154385.5f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 184.5f ) {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.676679730415f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 23.5f ) {
                                    return 1036.0/791.5;
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.104070216417f ) {
                                        if ( cl->size() <= 44.5f ) {
                                            if ( cl->stats.dump_number <= 14.5f ) {
                                                return 1359.0/563.6;
                                            } else {
                                                return 825.0/628.7;
                                            }
                                        } else {
                                            return 1158.0/925.8;
                                        }
                                    } else {
                                        if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                            return 943.0/447.7;
                                        } else {
                                            if ( cl->size() <= 17.5f ) {
                                                return 857.0/329.6;
                                            } else {
                                                return 1002.0/315.4;
                                            }
                                        }
                                    }
                                }
                            } else {
                                return 954.0/264.5;
                            }
                        } else {
                            return 946.0/189.2;
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 306204.0f ) {
                            if ( cl->stats.size_rel <= 0.713681817055f ) {
                                if ( cl->size() <= 14.5f ) {
                                    return 1121.0/612.5;
                                } else {
                                    return 938.0/362.2;
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                    if ( cl->size() <= 29.5f ) {
                                        return 1383.0/333.7;
                                    } else {
                                        return 887.0/305.2;
                                    }
                                } else {
                                    return 1315.0/254.3;
                                }
                            }
                        } else {
                            if ( cl->stats.num_antecedents_rel <= 0.305918365717f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 462691.5f ) {
                                    return 962.0/199.4;
                                } else {
                                    return 956.0/317.4;
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 457581.0f ) {
                                    return 1081.0/201.4;
                                } else {
                                    return 1132.0/173.0;
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.glue <= 9.5f ) {
                if ( rdb0_last_touched_diff <= 60440.0f ) {
                    if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                        if ( cl->stats.num_overlap_literals_rel <= 0.40636485815f ) {
                            return 772.0/1499.6;
                        } else {
                            return 717.0/972.6;
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 132.5f ) {
                            if ( cl->stats.glue <= 6.5f ) {
                                return 779.0/995.0;
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.786131203175f ) {
                                    return 734.0/777.3;
                                } else {
                                    return 1002.0/742.7;
                                }
                            }
                        } else {
                            return 1320.0/659.3;
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_last_touched_diff <= 147639.5f ) {
                        if ( cl->size() <= 16.5f ) {
                            if ( cl->stats.glue <= 7.5f ) {
                                if ( cl->stats.glue <= 6.5f ) {
                                    return 1078.0/1047.9;
                                } else {
                                    return 787.0/602.3;
                                }
                            } else {
                                if ( cl->size() <= 11.5f ) {
                                    return 854.0/386.6;
                                } else {
                                    return 916.0/553.5;
                                }
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals <= 86.5f ) {
                                if ( cl->stats.glue_rel_queue <= 0.701663374901f ) {
                                    return 827.0/461.9;
                                } else {
                                    return 1282.0/443.6;
                                }
                            } else {
                                return 1072.0/280.8;
                            }
                        }
                    } else {
                        if ( cl->size() <= 11.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 236062.5f ) {
                                return 848.0/423.2;
                            } else {
                                return 765.0/535.1;
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.651011168957f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.307816207409f ) {
                                    return 939.0/404.9;
                                } else {
                                    return 910.0/272.7;
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 53.5f ) {
                                    return 912.0/270.6;
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.783741533756f ) {
                                        return 1346.0/209.6;
                                    } else {
                                        return 1033.0/227.9;
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.glue_rel_long <= 0.912307858467f ) {
                    if ( cl->stats.rdb1_last_touched_diff <= 50750.0f ) {
                        if ( cl->stats.num_overlap_literals <= 173.5f ) {
                            if ( cl->stats.dump_number <= 5.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 80.5f ) {
                                    return 722.0/667.4;
                                } else {
                                    return 1272.0/506.7;
                                }
                            } else {
                                return 673.0/838.3;
                            }
                        } else {
                            return 1139.0/301.1;
                        }
                    } else {
                        if ( cl->stats.glue_rel_queue <= 0.532012462616f ) {
                            return 869.0/439.5;
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 72.5f ) {
                                if ( cl->stats.size_rel <= 0.749693155289f ) {
                                    return 1527.0/443.6;
                                } else {
                                    return 1435.0/561.6;
                                }
                            } else {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 4.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.702757120132f ) {
                                        return 1243.0/470.0;
                                    } else {
                                        return 1010.0/252.3;
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 0.942624807358f ) {
                                        if ( cl->stats.glue_rel_long <= 0.733947217464f ) {
                                            return 953.0/181.1;
                                        } else {
                                            return 1639.0/425.3;
                                        }
                                    } else {
                                        if ( cl->stats.glue <= 13.5f ) {
                                            return 1129.0/209.6;
                                        } else {
                                            return 1156.0/148.5;
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 76.5f ) {
                            return 842.0/559.6;
                        } else {
                            if ( cl->stats.dump_number <= 11.5f ) {
                                if ( rdb0_last_touched_diff <= 18329.0f ) {
                                    return 903.0/307.3;
                                } else {
                                    if ( cl->stats.glue <= 19.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 32368.5f ) {
                                            return 1045.0/225.9;
                                        } else {
                                            return 977.0/158.7;
                                        }
                                    } else {
                                        return 1926.0/134.3;
                                    }
                                }
                            } else {
                                return 998.0/392.7;
                            }
                        }
                    } else {
                        if ( cl->stats.glue <= 16.5f ) {
                            if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                if ( cl->stats.glue_rel_long <= 1.22410213947f ) {
                                    if ( cl->size() <= 16.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.706922769547f ) {
                                            return 1060.0/286.9;
                                        } else {
                                            return 1732.0/282.8;
                                        }
                                    } else {
                                        if ( cl->stats.size_rel <= 1.28283882141f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 85543.0f ) {
                                                return 1710.0/569.7;
                                            } else {
                                                if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                                    return 1333.0/268.6;
                                                } else {
                                                    return 1450.0/435.4;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.size_rel <= 1.51983451843f ) {
                                                return 1150.0/158.7;
                                            } else {
                                                return 1557.0/394.7;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 8.26855754852f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.91830354929f ) {
                                            return 952.0/236.0;
                                        } else {
                                            return 962.0/168.9;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 55898.0f ) {
                                            return 999.0/203.5;
                                        } else {
                                            if ( cl->size() <= 23.5f ) {
                                                return 1039.0/63.1;
                                            } else {
                                                return 1139.0/138.4;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 5.55328798294f ) {
                                    return 1248.0/69.2;
                                } else {
                                    if ( cl->stats.glue_rel_long <= 1.11450314522f ) {
                                        return 1190.0/156.7;
                                    } else {
                                        return 1689.0/124.1;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 54315.5f ) {
                                if ( cl->stats.size_rel <= 1.60060381889f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.285388737917f ) {
                                        return 1448.0/380.5;
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 1.44009208679f ) {
                                            if ( cl->stats.rdb1_act_ranking_top_10 <= 4.5f ) {
                                                return 1446.0/168.9;
                                            } else {
                                                return 1106.0/203.5;
                                            }
                                        } else {
                                            return 1157.0/111.9;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 576.5f ) {
                                        return 978.0/130.2;
                                    } else {
                                        return 1145.0/26.5;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 1.1281774044f ) {
                                    if ( cl->stats.size_rel <= 1.64747095108f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 223499.0f ) {
                                            if ( cl->stats.num_overlap_literals <= 91.5f ) {
                                                return 940.0/191.3;
                                            } else {
                                                if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                                    return 1410.0/107.8;
                                                } else {
                                                    return 979.0/148.5;
                                                }
                                            }
                                        } else {
                                            return 1037.0/219.8;
                                        }
                                    } else {
                                        return 1193.0/65.1;
                                    }
                                } else {
                                    if ( cl->size() <= 96.5f ) {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 6.76588439941f ) {
                                            return 983.0/148.5;
                                        } else {
                                            if ( rdb0_last_touched_diff <= 289714.0f ) {
                                                if ( cl->stats.size_rel <= 1.48661327362f ) {
                                                    if ( cl->size() <= 56.5f ) {
                                                        if ( cl->stats.glue_rel_queue <= 1.21670198441f ) {
                                                            return 956.0/111.9;
                                                        } else {
                                                            if ( cl->stats.glue <= 21.5f ) {
                                                                return 999.0/71.2;
                                                            } else {
                                                                return 1027.0/54.9;
                                                            }
                                                        }
                                                    } else {
                                                        return 1027.0/146.5;
                                                    }
                                                } else {
                                                    return 1718.0/87.5;
                                                }
                                            } else {
                                                return 1723.0/209.6;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 138.871520996f ) {
                                            if ( cl->stats.glue_rel_queue <= 1.46388196945f ) {
                                                return 1633.0/32.6;
                                            } else {
                                                return 1023.0/48.8;
                                            }
                                        } else {
                                            return 1923.0/142.4;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

static double estimator_should_keep_short_conf2_cluster0_1(
    const CMSat::Clause* cl
    , const uint64_t sumConflicts
    , const uint32_t rdb0_last_touched_diff
    , const uint32_t rdb0_act_ranking
    , const uint32_t rdb0_act_ranking_top_10
) {

    double rdb_rel_used_for_uip_creation = 0;
    if (cl->stats.used_for_uip_creation > cl->stats.rdb1_used_for_uip_creation) {
        rdb_rel_used_for_uip_creation = 1.0;
    } else {
        rdb_rel_used_for_uip_creation = 0.0;
    }

    double rdb0_avg_confl;
    if (cl->stats.sum_delta_confl_uip1_used == 0) {
        rdb0_avg_confl = 0;
    } else {
        rdb0_avg_confl = ((double)cl->stats.sum_uip1_used)/((double)cl->stats.sum_delta_confl_uip1_used);
    }

    double rdb0_used_per_confl;
    if (sumConflicts-cl->stats.introduced_at_conflict == 0) {
        rdb0_used_per_confl = 0;
    } else {
        rdb0_used_per_confl = ((double)cl->stats.sum_uip1_used)/((double)sumConflicts-(double)cl->stats.introduced_at_conflict);
    }
    if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
        if ( cl->size() <= 12.5f ) {
            if ( rdb0_last_touched_diff <= 42431.5f ) {
                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                    if ( cl->stats.num_total_lits_antecedents <= 21.5f ) {
                        if ( rdb0_last_touched_diff <= 2556.5f ) {
                            if ( cl->stats.used_for_uip_creation <= 5.5f ) {
                                if ( cl->stats.glue <= 4.5f ) {
                                    return 490.0/3127.5;
                                } else {
                                    return 451.0/3241.4;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 5.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.353578984737f ) {
                                        return 79.0/2030.7;
                                    } else {
                                        return 107.0/2199.6;
                                    }
                                } else {
                                    return 145.0/2390.9;
                                }
                            }
                        } else {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.0881944745779f ) {
                                if ( rdb0_last_touched_diff <= 23680.5f ) {
                                    if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                        if ( cl->size() <= 5.5f ) {
                                            return 505.0/2042.9;
                                        } else {
                                            return 462.0/1520.0;
                                        }
                                    } else {
                                        return 418.0/2466.2;
                                    }
                                } else {
                                    return 684.0/2083.6;
                                }
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.153416484594f ) {
                                        if ( cl->stats.dump_number <= 23.5f ) {
                                            return 294.0/1513.9;
                                        } else {
                                            return 360.0/1406.0;
                                        }
                                    } else {
                                        return 423.0/1310.4;
                                    }
                                } else {
                                    return 369.0/3084.7;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                            if ( rdb0_last_touched_diff <= 25088.0f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 33085.0f ) {
                                    if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                        return 554.0/1788.6;
                                    } else {
                                        return 448.0/1902.5;
                                    }
                                } else {
                                    return 421.0/1214.8;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.153182774782f ) {
                                    return 539.0/1092.7;
                                } else {
                                    return 458.0/1178.1;
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.730558633804f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.091371320188f ) {
                                    return 334.0/2344.1;
                                } else {
                                    if ( cl->stats.glue <= 5.5f ) {
                                        return 288.0/2578.1;
                                    } else {
                                        return 262.0/2631.0;
                                    }
                                }
                            } else {
                                return 351.0/2004.3;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_total_lits_antecedents <= 32.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 18544.5f ) {
                            if ( cl->stats.dump_number <= 2.5f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.123850077391f ) {
                                    return 494.0/1060.1;
                                } else {
                                    return 432.0/1239.2;
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.523597955704f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0818467661738f ) {
                                        return 700.0/2075.5;
                                    } else {
                                        return 474.0/1975.8;
                                    }
                                } else {
                                    return 756.0/2132.5;
                                }
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals <= 5.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.145623505116f ) {
                                    if ( rdb0_last_touched_diff <= 34320.0f ) {
                                        return 448.0/1351.1;
                                    } else {
                                        return 503.0/1269.7;
                                    }
                                } else {
                                    return 604.0/1338.9;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0517476201057f ) {
                                    return 580.0/966.5;
                                } else {
                                    if ( rdb0_last_touched_diff <= 32945.5f ) {
                                        return 662.0/1662.4;
                                    } else {
                                        if ( cl->stats.glue <= 5.5f ) {
                                            return 487.0/1135.4;
                                        } else {
                                            return 585.0/1041.8;
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_queue <= 0.91198015213f ) {
                            if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                if ( cl->size() <= 8.5f ) {
                                    return 551.0/1418.2;
                                } else {
                                    return 935.0/1619.7;
                                }
                            } else {
                                return 675.0/893.3;
                            }
                        } else {
                            return 1169.0/997.0;
                        }
                    }
                }
            } else {
                if ( cl->stats.num_total_lits_antecedents <= 21.5f ) {
                    if ( cl->stats.num_antecedents_rel <= 0.192824736238f ) {
                        if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                            if ( rdb0_last_touched_diff <= 82059.0f ) {
                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                    return 459.0/1503.7;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 47916.5f ) {
                                        return 748.0/1587.1;
                                    } else {
                                        return 565.0/948.2;
                                    }
                                }
                            } else {
                                return 694.0/966.5;
                            }
                        } else {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 7.5f ) {
                                if ( cl->stats.glue <= 5.5f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0291653461754f ) {
                                        if ( rdb0_last_touched_diff <= 162834.5f ) {
                                            return 1206.0/1684.8;
                                        } else {
                                            return 754.0/663.3;
                                        }
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0965600311756f ) {
                                            return 1026.0/1727.5;
                                        } else {
                                            return 946.0/1229.0;
                                        }
                                    }
                                } else {
                                    if ( cl->size() <= 9.5f ) {
                                        if ( cl->stats.dump_number <= 20.5f ) {
                                            return 655.0/838.3;
                                        } else {
                                            return 718.0/732.5;
                                        }
                                    } else {
                                        return 861.0/765.1;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 15.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.093244984746f ) {
                                        return 673.0/756.9;
                                    } else {
                                        return 728.0/632.8;
                                    }
                                } else {
                                    return 1078.0/667.4;
                                }
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 115546.5f ) {
                            if ( cl->stats.glue <= 5.5f ) {
                                return 912.0/1440.6;
                            } else {
                                return 748.0/864.8;
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.444172799587f ) {
                                return 848.0/429.3;
                            } else {
                                return 791.0/620.6;
                            }
                        }
                    }
                } else {
                    if ( rdb0_last_touched_diff <= 100159.0f ) {
                        if ( cl->stats.glue <= 9.5f ) {
                            if ( cl->stats.size_rel <= 0.194086074829f ) {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                                    return 555.0/1233.1;
                                } else {
                                    return 1041.0/1316.5;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 61.5f ) {
                                    if ( cl->size() <= 9.5f ) {
                                        if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                                            return 549.0/1035.7;
                                        } else {
                                            return 1326.0/1479.3;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 50470.5f ) {
                                            return 1030.0/1237.1;
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 0.622885704041f ) {
                                                return 709.0/624.7;
                                            } else {
                                                return 789.0/581.9;
                                            }
                                        }
                                    }
                                } else {
                                    return 902.0/765.1;
                                }
                            }
                        } else {
                            return 1160.0/608.4;
                        }
                    } else {
                        if ( cl->stats.glue_rel_queue <= 0.803654849529f ) {
                            if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                if ( cl->stats.size_rel <= 0.124551154673f ) {
                                    return 713.0/848.5;
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.27951747179f ) {
                                        if ( rdb0_last_touched_diff <= 198794.0f ) {
                                            if ( cl->size() <= 9.5f ) {
                                                return 716.0/722.3;
                                            } else {
                                                return 939.0/549.4;
                                            }
                                        } else {
                                            return 1382.0/665.4;
                                        }
                                    } else {
                                        return 1194.0/976.7;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 15.5f ) {
                                    return 944.0/437.5;
                                } else {
                                    return 912.0/551.4;
                                }
                            }
                        } else {
                            if ( cl->stats.antec_num_total_lits_rel <= 1.1876155138f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 4.32662630081f ) {
                                    if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                        return 847.0/547.4;
                                    } else {
                                        return 1088.0/492.4;
                                    }
                                } else {
                                    return 864.0/291.0;
                                }
                            } else {
                                return 941.0/248.2;
                            }
                        }
                    }
                }
            }
        } else {
            if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                if ( rdb0_last_touched_diff <= 58394.0f ) {
                    if ( cl->stats.glue_rel_long <= 1.00002503395f ) {
                        if ( cl->stats.glue_rel_long <= 0.757750034332f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 96.5f ) {
                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                    if ( rdb0_last_touched_diff <= 27106.0f ) {
                                        return 713.0/1721.4;
                                    } else {
                                        if ( cl->stats.size_rel <= 0.578009128571f ) {
                                            return 788.0/1271.7;
                                        } else {
                                            return 585.0/1035.7;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 7.5f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 30.5f ) {
                                            return 696.0/1373.5;
                                        } else {
                                            return 606.0/872.9;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                                            if ( cl->stats.dump_number <= 5.5f ) {
                                                return 911.0/1021.5;
                                            } else {
                                                return 1140.0/1741.8;
                                            }
                                        } else {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.245797246695f ) {
                                                return 1075.0/1168.0;
                                            } else {
                                                return 892.0/795.6;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.249716550112f ) {
                                    return 683.0/1013.3;
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                        if ( cl->size() <= 42.5f ) {
                                            return 1029.0/1222.9;
                                        } else {
                                            return 840.0/634.9;
                                        }
                                    } else {
                                        return 1198.0/575.8;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.426387965679f ) {
                                if ( rdb0_last_touched_diff <= 39547.5f ) {
                                    if ( cl->stats.dump_number <= 3.5f ) {
                                        if ( cl->stats.glue_rel_long <= 0.876577734947f ) {
                                            return 735.0/618.6;
                                        } else {
                                            return 829.0/518.9;
                                        }
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.180414929986f ) {
                                            return 558.0/1017.4;
                                        } else {
                                            return 509.0/1056.1;
                                        }
                                    }
                                } else {
                                    if ( cl->size() <= 25.5f ) {
                                        return 891.0/783.4;
                                    } else {
                                        return 828.0/529.0;
                                    }
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                    return 863.0/1037.7;
                                } else {
                                    if ( cl->stats.glue <= 13.5f ) {
                                        if ( cl->stats.dump_number <= 4.5f ) {
                                            return 1503.0/801.7;
                                        } else {
                                            return 901.0/756.9;
                                        }
                                    } else {
                                        return 1789.0/461.9;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.260929226875f ) {
                            if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                return 826.0/1098.8;
                            } else {
                                return 1214.0/905.5;
                            }
                        } else {
                            if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                                if ( cl->stats.dump_number <= 5.5f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 16.0214977264f ) {
                                        return 1692.0/638.9;
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 1.09998905659f ) {
                                            return 933.0/234.0;
                                        } else {
                                            return 1341.0/179.1;
                                        }
                                    }
                                } else {
                                    return 801.0/940.1;
                                }
                            } else {
                                if ( cl->stats.glue <= 13.5f ) {
                                    if ( cl->stats.num_overlap_literals <= 103.5f ) {
                                        return 1755.0/736.6;
                                    } else {
                                        return 955.0/293.0;
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 1.28647375107f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 36189.0f ) {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.992747783661f ) {
                                                if ( rdb0_last_touched_diff <= 33095.0f ) {
                                                    return 984.0/217.7;
                                                } else {
                                                    return 855.0/270.6;
                                                }
                                            } else {
                                                return 1471.0/181.1;
                                            }
                                        } else {
                                            return 1416.0/197.4;
                                        }
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 38.2583084106f ) {
                                            return 1680.0/232.0;
                                        } else {
                                            return 1812.0/95.6;
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.glue_rel_long <= 0.872684180737f ) {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.00729917222634f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 145231.5f ) {
                                if ( rdb0_last_touched_diff <= 111612.5f ) {
                                    if ( cl->stats.size_rel <= 0.423618555069f ) {
                                        return 878.0/1123.2;
                                    } else {
                                        if ( rdb0_last_touched_diff <= 80377.5f ) {
                                            return 838.0/691.8;
                                        } else {
                                            return 884.0/651.1;
                                        }
                                    }
                                } else {
                                    return 1226.0/818.0;
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 56.5f ) {
                                    if ( cl->size() <= 15.5f ) {
                                        return 851.0/419.2;
                                    } else {
                                        return 1648.0/742.7;
                                    }
                                } else {
                                    return 1643.0/673.5;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                    return 1085.0/986.9;
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.641585528851f ) {
                                        if ( cl->stats.dump_number <= 10.5f ) {
                                            return 853.0/404.9;
                                        } else {
                                            return 1293.0/929.9;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.841302156448f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 81542.0f ) {
                                                return 922.0/561.6;
                                            } else {
                                                return 927.0/388.6;
                                            }
                                        } else {
                                            return 855.0/374.4;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 4.5f ) {
                                    if ( rdb0_last_touched_diff <= 193515.0f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 58.5f ) {
                                            return 937.0/510.7;
                                        } else {
                                            return 1508.0/573.8;
                                        }
                                    } else {
                                        return 1321.0/364.2;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.491354048252f ) {
                                        if ( cl->stats.glue_rel_long <= 0.40070348978f ) {
                                            return 840.0/421.2;
                                        } else {
                                            return 964.0/409.0;
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 37.5f ) {
                                            if ( cl->stats.rdb1_act_ranking_top_10 <= 8.5f ) {
                                                if ( cl->stats.num_overlap_literals_rel <= 0.266779124737f ) {
                                                    if ( rdb0_last_touched_diff <= 161545.0f ) {
                                                        return 1335.0/522.9;
                                                    } else {
                                                        if ( rdb0_last_touched_diff <= 281318.5f ) {
                                                            return 963.0/256.4;
                                                        } else {
                                                            return 958.0/189.2;
                                                        }
                                                    }
                                                } else {
                                                    return 996.0/380.5;
                                                }
                                            } else {
                                                return 1763.0/417.1;
                                            }
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.26627856493f ) {
                                                return 1386.0/221.8;
                                            } else {
                                                if ( cl->stats.rdb1_act_ranking_top_10 <= 8.5f ) {
                                                    if ( cl->stats.num_overlap_literals <= 57.5f ) {
                                                        return 972.0/195.3;
                                                    } else {
                                                        if ( cl->stats.num_total_lits_antecedents <= 154.5f ) {
                                                            return 1028.0/352.0;
                                                        } else {
                                                            if ( cl->stats.antec_num_total_lits_rel <= 1.15838563442f ) {
                                                                return 1001.0/219.8;
                                                            } else {
                                                                return 903.0/252.3;
                                                            }
                                                        }
                                                    }
                                                } else {
                                                    return 1626.0/293.0;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.antec_num_total_lits_rel <= 0.640827417374f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 130176.0f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 3.68990373611f ) {
                                    if ( cl->stats.glue_rel_long <= 1.0401403904f ) {
                                        if ( cl->stats.dump_number <= 11.5f ) {
                                            return 1130.0/455.8;
                                        } else {
                                            return 821.0/628.7;
                                        }
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.401202201843f ) {
                                            return 1535.0/616.5;
                                        } else {
                                            return 886.0/280.8;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 1.21313452721f ) {
                                        if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                            return 1271.0/413.1;
                                        } else {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 23.0200004578f ) {
                                                return 1478.0/390.7;
                                            } else {
                                                return 1054.0/136.3;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.dump_number <= 9.5f ) {
                                            return 1210.0/99.7;
                                        } else {
                                            return 999.0/156.7;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 40.5f ) {
                                    if ( cl->stats.num_overlap_literals <= 13.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0556072480977f ) {
                                            return 1059.0/366.3;
                                        } else {
                                            return 1308.0/274.7;
                                        }
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 11.4699993134f ) {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.482605338097f ) {
                                                if ( cl->stats.size_rel <= 0.881614565849f ) {
                                                    return 1408.0/394.7;
                                                } else {
                                                    return 970.0/193.3;
                                                }
                                            } else {
                                                return 978.0/150.6;
                                            }
                                        } else {
                                            return 1327.0/189.2;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 38.5f ) {
                                        if ( rdb0_last_touched_diff <= 181009.0f ) {
                                            return 1252.0/175.0;
                                        } else {
                                            if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                                return 1118.0/32.6;
                                            } else {
                                                return 1261.0/136.3;
                                            }
                                        }
                                    } else {
                                        return 948.0/189.2;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.glue <= 15.5f ) {
                                if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                    if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                        return 1593.0/413.1;
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 1.25510418415f ) {
                                            if ( rdb0_last_touched_diff <= 118624.5f ) {
                                                if ( cl->stats.num_total_lits_antecedents <= 121.5f ) {
                                                    return 919.0/293.0;
                                                } else {
                                                    return 910.0/225.9;
                                                }
                                            } else {
                                                if ( cl->stats.dump_number <= 18.5f ) {
                                                    return 1088.0/126.2;
                                                } else {
                                                    if ( cl->stats.antecedents_glue_long_reds_var <= 7.72623968124f ) {
                                                        return 1086.0/276.7;
                                                    } else {
                                                        return 1093.0/189.2;
                                                    }
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 1.94034481049f ) {
                                                return 1720.0/211.6;
                                            } else {
                                                return 1249.0/219.8;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 1.02085638046f ) {
                                        return 1536.0/170.9;
                                    } else {
                                        return 1321.0/81.4;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 1.10613918304f ) {
                                    if ( cl->stats.num_overlap_literals <= 354.5f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 211.5f ) {
                                            return 1315.0/232.0;
                                        } else {
                                            return 1910.0/258.4;
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 1.95916795731f ) {
                                            return 1273.0/61.0;
                                        } else {
                                            return 1752.0/227.9;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 9.5f ) {
                                        if ( cl->stats.size_rel <= 2.42558646202f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.9152572155f ) {
                                                if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                                    return 1297.0/142.4;
                                                } else {
                                                    return 1156.0/207.5;
                                                }
                                            } else {
                                                if ( cl->stats.num_overlap_literals_rel <= 3.17700123787f ) {
                                                    if ( cl->stats.rdb1_last_touched_diff <= 68268.0f ) {
                                                        return 993.0/144.5;
                                                    } else {
                                                        if ( cl->stats.num_overlap_literals_rel <= 1.39310383797f ) {
                                                            return 1775.0/105.8;
                                                        } else {
                                                            if ( rdb0_last_touched_diff <= 149795.5f ) {
                                                                return 1209.0/126.2;
                                                            } else {
                                                                return 1581.0/124.1;
                                                            }
                                                        }
                                                    }
                                                } else {
                                                    return 1648.0/91.6;
                                                }
                                            }
                                        } else {
                                            return 1904.0/105.8;
                                        }
                                    } else {
                                        return 1767.0/42.7;
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.antecedents_glue_long_reds_var <= 0.258913695812f ) {
                    if ( rdb0_last_touched_diff <= 2379.5f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 78.5f ) {
                            if ( rdb0_last_touched_diff <= 909.0f ) {
                                return 188.0/2598.4;
                            } else {
                                return 272.0/1703.1;
                            }
                        } else {
                            return 170.0/1790.6;
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 21860.5f ) {
                            return 594.0/2555.7;
                        } else {
                            return 896.0/2240.3;
                        }
                    }
                } else {
                    if ( cl->stats.antecedents_glue_long_reds_var <= 8.49489784241f ) {
                        if ( cl->stats.glue_rel_long <= 0.822581768036f ) {
                            if ( cl->stats.size_rel <= 0.807448983192f ) {
                                if ( rdb0_last_touched_diff <= 2693.5f ) {
                                    return 396.0/2421.4;
                                } else {
                                    return 698.0/1943.2;
                                }
                            } else {
                                return 458.0/1467.1;
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 83.5f ) {
                                return 471.0/1292.1;
                            } else {
                                return 491.0/1253.4;
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 2200.0f ) {
                            return 317.0/2179.3;
                        } else {
                            return 740.0/2061.2;
                        }
                    }
                }
            }
        }
    } else {
        if ( cl->stats.antec_num_total_lits_rel <= 0.686045050621f ) {
            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                    if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                        if ( cl->stats.glue_rel_long <= 1.00092697144f ) {
                            if ( cl->size() <= 10.5f ) {
                                if ( cl->stats.glue_rel_queue <= 0.387409299612f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                        return 449.0/1397.9;
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 16.5f ) {
                                            return 473.0/2879.2;
                                        } else {
                                            return 347.0/1617.7;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.164608359337f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 3048.5f ) {
                                                return 264.0/1548.5;
                                            } else {
                                                return 597.0/2525.2;
                                            }
                                        } else {
                                            return 463.0/1782.5;
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 8.5f ) {
                                            return 373.0/2649.3;
                                        } else {
                                            return 208.0/1774.3;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 6.32227897644f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.285761833191f ) {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                                            if ( rdb0_last_touched_diff <= 13982.0f ) {
                                                return 438.0/1239.2;
                                            } else {
                                                return 542.0/1263.6;
                                            }
                                        } else {
                                            return 478.0/2356.3;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                            return 963.0/1955.4;
                                        } else {
                                            return 625.0/2350.2;
                                        }
                                    }
                                } else {
                                    return 763.0/1583.1;
                                }
                            }
                        } else {
                            return 886.0/1442.7;
                        }
                    } else {
                        if ( cl->stats.dump_number <= 1.5f ) {
                            return 364.0/1548.5;
                        } else {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.217097043991f ) {
                                if ( rdb0_last_touched_diff <= 7338.5f ) {
                                    if ( rdb0_last_touched_diff <= 916.5f ) {
                                        return 188.0/1794.7;
                                    } else {
                                        if ( cl->stats.glue <= 4.5f ) {
                                            return 373.0/3037.9;
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 24.5f ) {
                                                return 297.0/2274.9;
                                            } else {
                                                return 315.0/1544.4;
                                            }
                                        }
                                    }
                                } else {
                                    return 466.0/2549.6;
                                }
                            } else {
                                return 555.0/3035.9;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.used_for_uip_creation <= 8.5f ) {
                        if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                            if ( rdb0_last_touched_diff <= 1145.5f ) {
                                if ( cl->stats.glue <= 6.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0639148801565f ) {
                                        return 184.0/2879.2;
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.424578368664f ) {
                                            return 65.0/2553.7;
                                        } else {
                                            return 117.0/2616.7;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.160512924194f ) {
                                        return 142.0/1808.9;
                                    } else {
                                        return 95.0/1845.5;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 7.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.723106443882f ) {
                                        if ( rdb0_last_touched_diff <= 2957.5f ) {
                                            return 291.0/3298.4;
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 18.5f ) {
                                                return 242.0/3096.9;
                                            } else {
                                                return 440.0/2893.5;
                                            }
                                        }
                                    } else {
                                        return 351.0/2148.7;
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.0681616961956f ) {
                                        return 296.0/3206.8;
                                    } else {
                                        if ( cl->size() <= 8.5f ) {
                                            if ( cl->stats.glue_rel_long <= 0.425236731768f ) {
                                                return 188.0/3768.4;
                                            } else {
                                                return 108.0/2773.4;
                                            }
                                        } else {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.18800996244f ) {
                                                return 174.0/2795.8;
                                            } else {
                                                return 209.0/2720.5;
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                if ( rdb0_last_touched_diff <= 1086.0f ) {
                                    return 250.0/2567.9;
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.611415028572f ) {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                            return 521.0/3044.0;
                                        } else {
                                            return 200.0/1782.5;
                                        }
                                    } else {
                                        return 472.0/2488.5;
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 4865.5f ) {
                                    if ( cl->size() <= 20.5f ) {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0507688559592f ) {
                                                return 191.0/1951.4;
                                            } else {
                                                return 169.0/2500.7;
                                            }
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.106152005494f ) {
                                                return 126.0/1847.6;
                                            } else {
                                                return 147.0/3196.6;
                                            }
                                        }
                                    } else {
                                        return 224.0/2134.5;
                                    }
                                } else {
                                    return 323.0/2116.2;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.106590613723f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 4009.0f ) {
                                if ( rdb0_last_touched_diff <= 4006.0f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0847829282284f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 13.5f ) {
                                            if ( rdb0_last_touched_diff <= 1093.0f ) {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.00815433450043f ) {
                                                    return 11.0/2338.0;
                                                } else {
                                                    if ( cl->stats.rdb1_used_for_uip_creation <= 10.5f ) {
                                                        return 53.0/2193.5;
                                                    } else {
                                                        if ( cl->stats.rdb1_last_touched_diff <= 830.5f ) {
                                                            if ( cl->stats.num_overlap_literals_rel <= 0.0214820429683f ) {
                                                                return 44.0/2549.6;
                                                            } else {
                                                                return 15.0/2099.9;
                                                            }
                                                        } else {
                                                            return 51.0/2262.7;
                                                        }
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.size_rel <= 0.113990806043f ) {
                                                    return 91.0/2293.2;
                                                } else {
                                                    return 72.0/2738.8;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 0.316962212324f ) {
                                                if ( cl->stats.num_antecedents_rel <= 0.0851788669825f ) {
                                                    if ( cl->stats.num_overlap_literals_rel <= 0.0136069711298f ) {
                                                        return 78.0/3646.3;
                                                    } else {
                                                        return 78.0/2197.6;
                                                    }
                                                } else {
                                                    return 132.0/2635.0;
                                                }
                                            } else {
                                                if ( cl->stats.size_rel <= 0.202883601189f ) {
                                                    if ( cl->stats.rdb1_used_for_uip_creation <= 15.5f ) {
                                                        return 71.0/1961.5;
                                                    } else {
                                                        if ( cl->stats.rdb1_used_for_uip_creation <= 41.5f ) {
                                                            return 25.0/2053.1;
                                                        } else {
                                                            return 40.0/2179.3;
                                                        }
                                                    }
                                                } else {
                                                    return 99.0/2407.1;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 20.5f ) {
                                            if ( rdb0_last_touched_diff <= 165.5f ) {
                                                return 30.0/2769.3;
                                            } else {
                                                if ( cl->size() <= 34.5f ) {
                                                    if ( cl->stats.num_overlap_literals <= 5.5f ) {
                                                        return 55.0/3440.8;
                                                    } else {
                                                        if ( cl->stats.num_antecedents_rel <= 0.286736607552f ) {
                                                            return 112.0/3485.6;
                                                        } else {
                                                            return 39.0/2018.5;
                                                        }
                                                    }
                                                } else {
                                                    return 83.0/2266.7;
                                                }
                                            }
                                        } else {
                                            if ( rdb0_last_touched_diff <= 1459.5f ) {
                                                if ( cl->stats.glue_rel_queue <= 0.32475066185f ) {
                                                    return 33.0/2486.5;
                                                } else {
                                                    if ( cl->stats.num_total_lits_antecedents <= 70.5f ) {
                                                        if ( cl->stats.num_overlap_literals <= 7.5f ) {
                                                            return 12.0/3231.2;
                                                        } else {
                                                            return 6.0/2820.2;
                                                        }
                                                    } else {
                                                        return 23.0/3514.1;
                                                    }
                                                }
                                            } else {
                                                return 57.0/2696.1;
                                            }
                                        }
                                    }
                                } else {
                                    if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                        return 124.0/2476.3;
                                    } else {
                                        return 192.0/2594.3;
                                    }
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.0435643903911f ) {
                                    return 175.0/2893.5;
                                } else {
                                    if ( rdb0_last_touched_diff <= 793.5f ) {
                                        return 84.0/3959.7;
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.436670482159f ) {
                                            return 123.0/2207.7;
                                        } else {
                                            return 81.0/2038.9;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 11.5f ) {
                                if ( rdb0_last_touched_diff <= 596.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.146237209439f ) {
                                        return 60.0/2000.2;
                                    } else {
                                        return 38.0/2211.8;
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 3713.0f ) {
                                        if ( cl->stats.glue <= 5.5f ) {
                                            return 88.0/1961.5;
                                        } else {
                                            return 237.0/3658.5;
                                        }
                                    } else {
                                        return 182.0/1902.5;
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 3663.0f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 2085.5f ) {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 35.5f ) {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 24.5f ) {
                                                if ( rdb0_last_touched_diff <= 675.5f ) {
                                                    return 21.0/2010.4;
                                                } else {
                                                    return 57.0/2065.3;
                                                }
                                            } else {
                                                return 60.0/2154.8;
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 86.5f ) {
                                                return 34.0/3748.1;
                                            } else {
                                                return 39.0/2248.4;
                                            }
                                        }
                                    } else {
                                        return 110.0/3123.4;
                                    }
                                } else {
                                    return 133.0/1857.8;
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                    if ( cl->size() <= 11.5f ) {
                        if ( cl->stats.glue_rel_long <= 0.62109541893f ) {
                            if ( cl->size() <= 6.5f ) {
                                if ( rdb0_last_touched_diff <= 15324.0f ) {
                                    if ( cl->stats.num_overlap_literals <= 5.5f ) {
                                        return 350.0/1941.2;
                                    } else {
                                        return 309.0/1540.3;
                                    }
                                } else {
                                    return 624.0/2321.7;
                                }
                            } else {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                    return 693.0/1798.7;
                                } else {
                                    return 416.0/2177.2;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                return 825.0/1817.1;
                            } else {
                                return 329.0/1444.7;
                            }
                        }
                    } else {
                        if ( cl->stats.dump_number <= 1.5f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 130.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.261466056108f ) {
                                    return 1034.0/1186.3;
                                } else {
                                    return 967.0/724.4;
                                }
                            } else {
                                return 983.0/236.0;
                            }
                        } else {
                            if ( cl->size() <= 61.5f ) {
                                if ( cl->stats.num_overlap_literals <= 18.5f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                                        return 845.0/1890.3;
                                    } else {
                                        return 249.0/1609.5;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.236793756485f ) {
                                        return 520.0/1031.6;
                                    } else {
                                        return 508.0/1214.8;
                                    }
                                }
                            } else {
                                return 610.0/960.4;
                            }
                        }
                    }
                } else {
                    if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                        if ( cl->stats.glue_rel_queue <= 0.572485387325f ) {
                            if ( rdb0_last_touched_diff <= 4130.5f ) {
                                return 196.0/3046.1;
                            } else {
                                if ( rdb0_last_touched_diff <= 6822.5f ) {
                                    return 285.0/2551.6;
                                } else {
                                    return 392.0/2645.2;
                                }
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals <= 9.5f ) {
                                return 216.0/2057.2;
                            } else {
                                return 538.0/2810.0;
                            }
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 30.5f ) {
                            return 247.0/1837.4;
                        } else {
                            return 399.0/1526.1;
                        }
                    }
                }
            }
        } else {
            if ( rdb0_last_touched_diff <= 9798.5f ) {
                if ( cl->stats.glue_rel_long <= 0.905970275402f ) {
                    if ( cl->stats.dump_number <= 4.5f ) {
                        return 285.0/2132.5;
                    } else {
                        return 215.0/2744.9;
                    }
                } else {
                    return 494.0/3219.0;
                }
            } else {
                if ( cl->stats.glue_rel_queue <= 0.941700220108f ) {
                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                        return 648.0/1721.4;
                    } else {
                        return 991.0/1355.2;
                    }
                } else {
                    if ( cl->stats.num_total_lits_antecedents <= 370.5f ) {
                        if ( rdb0_last_touched_diff <= 15764.5f ) {
                            return 1201.0/907.5;
                        } else {
                            return 858.0/396.8;
                        }
                    } else {
                        if ( cl->stats.glue_rel_long <= 1.19149136543f ) {
                            return 946.0/372.4;
                        } else {
                            return 981.0/146.5;
                        }
                    }
                }
            }
        }
    }
}

static double estimator_should_keep_short_conf2_cluster0_2(
    const CMSat::Clause* cl
    , const uint64_t sumConflicts
    , const uint32_t rdb0_last_touched_diff
    , const uint32_t rdb0_act_ranking
    , const uint32_t rdb0_act_ranking_top_10
) {

    double rdb_rel_used_for_uip_creation = 0;
    if (cl->stats.used_for_uip_creation > cl->stats.rdb1_used_for_uip_creation) {
        rdb_rel_used_for_uip_creation = 1.0;
    } else {
        rdb_rel_used_for_uip_creation = 0.0;
    }

    double rdb0_avg_confl;
    if (cl->stats.sum_delta_confl_uip1_used == 0) {
        rdb0_avg_confl = 0;
    } else {
        rdb0_avg_confl = ((double)cl->stats.sum_uip1_used)/((double)cl->stats.sum_delta_confl_uip1_used);
    }

    double rdb0_used_per_confl;
    if (sumConflicts-cl->stats.introduced_at_conflict == 0) {
        rdb0_used_per_confl = 0;
    } else {
        rdb0_used_per_confl = ((double)cl->stats.sum_uip1_used)/((double)sumConflicts-(double)cl->stats.introduced_at_conflict);
    }
    if ( rdb0_act_ranking_top_10 <= 2.5f ) {
        if ( cl->stats.used_for_uip_creation <= 0.5f ) {
            if ( cl->stats.glue_rel_queue <= 1.00020003319f ) {
                if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                    if ( cl->size() <= 11.5f ) {
                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 19522.0f ) {
                                if ( cl->stats.glue <= 6.5f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.505689799786f ) {
                                            if ( cl->stats.num_overlap_literals <= 6.5f ) {
                                                return 435.0/1654.3;
                                            } else {
                                                return 411.0/1353.1;
                                            }
                                        } else {
                                            return 371.0/1707.2;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.381024956703f ) {
                                            return 440.0/1361.3;
                                        } else {
                                            return 686.0/2541.4;
                                        }
                                    }
                                } else {
                                    return 787.0/2362.4;
                                }
                            } else {
                                if ( cl->stats.glue <= 4.5f ) {
                                    return 680.0/2303.4;
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 6.5f ) {
                                        return 548.0/1424.3;
                                    } else {
                                        if ( cl->stats.glue <= 6.5f ) {
                                            return 498.0/1149.7;
                                        } else {
                                            return 568.0/1009.3;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 33877.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 2.49244403839f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 14.5f ) {
                                        if ( cl->stats.size_rel <= 0.199828237295f ) {
                                            return 847.0/2177.2;
                                        } else {
                                            return 500.0/1959.5;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.714395403862f ) {
                                            if ( cl->stats.glue_rel_long <= 0.338283896446f ) {
                                                return 476.0/1235.1;
                                            } else {
                                                if ( cl->stats.rdb1_last_touched_diff <= 19378.5f ) {
                                                    if ( cl->stats.antec_num_total_lits_rel <= 0.179706633091f ) {
                                                        return 621.0/1332.8;
                                                    } else {
                                                        return 423.0/1194.4;
                                                    }
                                                } else {
                                                    return 767.0/1414.2;
                                                }
                                            }
                                        } else {
                                            return 415.0/1292.1;
                                        }
                                    }
                                } else {
                                    return 538.0/966.5;
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 67151.5f ) {
                                    if ( cl->stats.num_overlap_literals <= 8.5f ) {
                                        return 999.0/1719.4;
                                    } else {
                                        return 922.0/1385.7;
                                    }
                                } else {
                                    return 1163.0/1283.9;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 12.0586414337f ) {
                            if ( rdb0_last_touched_diff <= 43938.5f ) {
                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                    if ( cl->stats.dump_number <= 3.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 15372.0f ) {
                                            return 1140.0/1861.8;
                                        } else {
                                            return 903.0/921.8;
                                        }
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 35.5f ) {
                                            return 824.0/2173.1;
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 17260.0f ) {
                                                if ( cl->stats.glue_rel_long <= 0.59571146965f ) {
                                                    return 540.0/1035.7;
                                                } else {
                                                    return 708.0/1471.1;
                                                }
                                            } else {
                                                return 990.0/1603.4;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 38.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.182218968868f ) {
                                            return 563.0/1131.3;
                                        } else {
                                            return 523.0/958.4;
                                        }
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.0424451604486f ) {
                                            return 644.0/927.9;
                                        } else {
                                            if ( cl->stats.dump_number <= 5.5f ) {
                                                return 1490.0/1074.4;
                                            } else {
                                                return 1052.0/1412.1;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->size() <= 43.5f ) {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.216533854604f ) {
                                            if ( cl->stats.num_total_lits_antecedents <= 39.5f ) {
                                                return 668.0/1133.4;
                                            } else {
                                                return 731.0/891.2;
                                            }
                                        } else {
                                            return 969.0/818.0;
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 61972.5f ) {
                                            return 962.0/834.3;
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 49.5f ) {
                                                return 846.0/604.3;
                                            } else {
                                                return 1062.0/602.3;
                                            }
                                        }
                                    }
                                } else {
                                    return 1084.0/563.6;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 30556.0f ) {
                                if ( cl->stats.glue_rel_queue <= 0.881253898144f ) {
                                    return 874.0/1054.0;
                                } else {
                                    return 846.0/616.5;
                                }
                            } else {
                                if ( cl->stats.glue <= 14.5f ) {
                                    return 791.0/551.4;
                                } else {
                                    return 964.0/382.5;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                        if ( cl->stats.glue <= 6.5f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 21.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 12.5f ) {
                                    return 405.0/1680.7;
                                } else {
                                    return 452.0/2158.9;
                                }
                            } else {
                                return 803.0/2441.7;
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 41.5f ) {
                                return 737.0/2248.4;
                            } else {
                                if ( cl->stats.num_overlap_literals <= 37.5f ) {
                                    return 582.0/999.1;
                                } else {
                                    return 669.0/1330.7;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 2395.0f ) {
                            if ( cl->size() <= 7.5f ) {
                                return 364.0/3249.5;
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 1021.5f ) {
                                    return 389.0/2447.8;
                                } else {
                                    return 538.0/2563.8;
                                }
                            }
                        } else {
                            if ( cl->size() <= 10.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0354793891311f ) {
                                    return 460.0/1992.1;
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0929561182857f ) {
                                        return 312.0/2260.6;
                                    } else {
                                        return 350.0/1953.4;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 7.5f ) {
                                    return 784.0/2171.1;
                                } else {
                                    return 544.0/2242.3;
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.num_overlap_literals_rel <= 0.315888792276f ) {
                    if ( cl->stats.rdb1_last_touched_diff <= 26485.0f ) {
                        if ( cl->stats.size_rel <= 0.76516354084f ) {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.156369328499f ) {
                                return 486.0/1060.1;
                            } else {
                                return 662.0/1153.7;
                            }
                        } else {
                            return 958.0/1141.5;
                        }
                    } else {
                        return 1622.0/895.3;
                    }
                } else {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 8.51711750031f ) {
                            if ( rdb0_last_touched_diff <= 46624.5f ) {
                                if ( cl->stats.num_overlap_literals <= 86.5f ) {
                                    return 784.0/655.2;
                                } else {
                                    return 998.0/549.4;
                                }
                            } else {
                                return 869.0/280.8;
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 45717.5f ) {
                                if ( cl->stats.num_overlap_literals <= 454.5f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 21.5532875061f ) {
                                        return 1043.0/518.9;
                                    } else {
                                        return 1249.0/407.0;
                                    }
                                } else {
                                    return 1003.0/140.4;
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 1.21189498901f ) {
                                    return 1380.0/221.8;
                                } else {
                                    return 1060.0/63.1;
                                }
                            }
                        }
                    } else {
                        return 690.0/913.6;
                    }
                }
            }
        } else {
            if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                    if ( cl->stats.rdb1_last_touched_diff <= 40804.5f ) {
                        if ( cl->stats.glue_rel_long <= 0.78436088562f ) {
                            if ( cl->stats.num_overlap_literals <= 15.5f ) {
                                if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.0546875f ) {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                                if ( cl->size() <= 9.5f ) {
                                                    if ( rdb0_last_touched_diff <= 4136.0f ) {
                                                        if ( cl->stats.size_rel <= 0.238722324371f ) {
                                                            return 251.0/1802.8;
                                                        } else {
                                                            return 157.0/1735.7;
                                                        }
                                                    } else {
                                                        return 569.0/2962.6;
                                                    }
                                                } else {
                                                    return 305.0/1650.2;
                                                }
                                            } else {
                                                if ( cl->stats.glue_rel_long <= 0.37980735302f ) {
                                                    return 308.0/2128.4;
                                                } else {
                                                    return 272.0/2732.7;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                                                return 344.0/1666.5;
                                            } else {
                                                return 468.0/2720.5;
                                            }
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 5021.5f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0595391206443f ) {
                                                return 574.0/2747.0;
                                            } else {
                                                return 285.0/2238.3;
                                            }
                                        } else {
                                            return 396.0/1424.3;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 6.5f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.0655530393124f ) {
                                            if ( rdb0_last_touched_diff <= 1230.0f ) {
                                                return 130.0/2372.6;
                                            } else {
                                                return 317.0/2419.4;
                                            }
                                        } else {
                                            if ( cl->stats.used_for_uip_creation <= 9.5f ) {
                                                if ( rdb0_last_touched_diff <= 2536.5f ) {
                                                    if ( rdb0_last_touched_diff <= 635.5f ) {
                                                        return 103.0/2124.3;
                                                    } else {
                                                        return 181.0/2879.2;
                                                    }
                                                } else {
                                                    return 304.0/2985.0;
                                                }
                                            } else {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.10542678833f ) {
                                                    if ( cl->stats.num_antecedents_rel <= 0.113194927573f ) {
                                                        return 120.0/1990.0;
                                                    } else {
                                                        return 85.0/1992.1;
                                                    }
                                                } else {
                                                    return 65.0/2637.1;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.used_for_uip_creation <= 7.5f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.101484201849f ) {
                                                return 359.0/2844.6;
                                            } else {
                                                return 150.0/1865.9;
                                            }
                                        } else {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0379687771201f ) {
                                                return 180.0/1937.1;
                                            } else {
                                                if ( cl->size() <= 7.5f ) {
                                                    return 78.0/1963.6;
                                                } else {
                                                    return 181.0/2936.2;
                                                }
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0933679863811f ) {
                                    if ( cl->stats.used_for_uip_creation <= 6.5f ) {
                                        return 604.0/2352.2;
                                    } else {
                                        return 181.0/1914.7;
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 0.380278229713f ) {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 1.45959997177f ) {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                                                return 218.0/2034.8;
                                            } else {
                                                return 214.0/2472.3;
                                            }
                                        } else {
                                            return 258.0/1778.4;
                                        }
                                    } else {
                                        if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                            if ( cl->size() <= 20.5f ) {
                                                return 449.0/2250.5;
                                            } else {
                                                return 496.0/1513.9;
                                            }
                                        } else {
                                            if ( cl->stats.used_for_uip_creation <= 8.5f ) {
                                                return 296.0/2307.4;
                                            } else {
                                                return 111.0/2097.9;
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 86.5f ) {
                                if ( rdb0_last_touched_diff <= 2922.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.890766501427f ) {
                                        return 309.0/2234.2;
                                    } else {
                                        if ( rdb0_last_touched_diff <= 941.5f ) {
                                            return 147.0/2055.1;
                                        } else {
                                            return 261.0/1947.3;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.10423719883f ) {
                                        return 453.0/2468.2;
                                    } else {
                                        return 488.0/2087.7;
                                    }
                                }
                            } else {
                                if ( cl->size() <= 87.5f ) {
                                    if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                        return 761.0/2537.4;
                                    } else {
                                        return 187.0/1916.8;
                                    }
                                } else {
                                    return 396.0/1326.7;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_queue <= 0.631968736649f ) {
                            if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 102600.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 59115.5f ) {
                                        return 414.0/1570.9;
                                    } else {
                                        return 433.0/1418.2;
                                    }
                                } else {
                                    return 458.0/1261.6;
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 1736.0f ) {
                                    return 200.0/2917.9;
                                } else {
                                    return 291.0/1819.1;
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.874977827072f ) {
                                if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                                    return 518.0/1035.7;
                                } else {
                                    return 481.0/2620.8;
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 2528.0f ) {
                                    return 325.0/1454.9;
                                } else {
                                    return 540.0/1027.6;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                        if ( cl->stats.glue_rel_long <= 0.512097597122f ) {
                            return 457.0/1548.5;
                        } else {
                            return 887.0/1749.9;
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.149444445968f ) {
                            return 498.0/2639.1;
                        } else {
                            return 489.0/1501.7;
                        }
                    }
                }
            } else {
                if ( cl->stats.antecedents_glue_long_reds_var <= 0.0943827182055f ) {
                    if ( cl->stats.used_for_uip_creation <= 9.5f ) {
                        if ( cl->stats.num_overlap_literals_rel <= 0.0315519087017f ) {
                            if ( rdb0_last_touched_diff <= 4575.0f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 1751.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 537.5f ) {
                                        return 158.0/2138.6;
                                    } else {
                                        return 171.0/2981.0;
                                    }
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                        return 271.0/2287.1;
                                    } else {
                                        return 146.0/2268.8;
                                    }
                                }
                            } else {
                                if ( cl->stats.size_rel <= 0.130226358771f ) {
                                    return 174.0/2049.0;
                                } else {
                                    return 321.0/2051.1;
                                }
                            }
                        } else {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.199483707547f ) {
                                if ( cl->size() <= 8.5f ) {
                                    if ( cl->stats.size_rel <= 0.32380437851f ) {
                                        if ( cl->stats.glue <= 4.5f ) {
                                            if ( rdb0_last_touched_diff <= 5463.0f ) {
                                                if ( rdb0_last_touched_diff <= 902.0f ) {
                                                    return 48.0/2059.2;
                                                } else {
                                                    if ( cl->stats.dump_number <= 21.5f ) {
                                                        return 148.0/2388.8;
                                                    } else {
                                                        return 80.0/1935.1;
                                                    }
                                                }
                                            } else {
                                                return 167.0/1782.5;
                                            }
                                        } else {
                                            return 154.0/3155.9;
                                        }
                                    } else {
                                        return 105.0/3231.2;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 8.5f ) {
                                        return 96.0/1916.8;
                                    } else {
                                        return 203.0/2217.9;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 11.5f ) {
                                    return 294.0/3243.4;
                                } else {
                                    return 97.0/1908.6;
                                }
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 4129.5f ) {
                            if ( cl->stats.num_overlap_literals_rel <= 0.0253083743155f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.00447295047343f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.257069587708f ) {
                                        return 24.0/2388.8;
                                    } else {
                                        return 69.0/3438.8;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 461.5f ) {
                                        if ( cl->stats.size_rel <= 0.0436227433383f ) {
                                            return 25.0/2057.2;
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.0410940051079f ) {
                                                return 99.0/2574.0;
                                            } else {
                                                return 79.0/3876.3;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 14.5f ) {
                                            if ( cl->stats.glue_rel_long <= 0.328211635351f ) {
                                                return 122.0/2203.7;
                                            } else {
                                                return 80.0/2089.7;
                                            }
                                        } else {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.00898994319141f ) {
                                                return 78.0/1924.9;
                                            } else {
                                                if ( rdb0_last_touched_diff <= 767.5f ) {
                                                    return 53.0/2710.3;
                                                } else {
                                                    return 63.0/1916.8;
                                                }
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 1031.5f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 9.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0711100846529f ) {
                                            return 58.0/2759.2;
                                        } else {
                                            return 67.0/1926.9;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.207752808928f ) {
                                            return 43.0/2315.6;
                                        } else {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.0815036520362f ) {
                                                if ( cl->stats.num_overlap_literals <= 5.5f ) {
                                                    return 24.0/2340.0;
                                                } else {
                                                    return 45.0/2252.5;
                                                }
                                            } else {
                                                if ( rdb0_last_touched_diff <= 525.5f ) {
                                                    if ( cl->stats.used_for_uip_creation <= 23.5f ) {
                                                        return 4.0/2610.6;
                                                    } else {
                                                        if ( cl->stats.glue_rel_queue <= 0.646066308022f ) {
                                                            if ( cl->stats.rdb1_last_touched_diff <= 198.0f ) {
                                                                return 14.0/2173.1;
                                                            } else {
                                                                return 9.0/3369.6;
                                                            }
                                                        } else {
                                                            return 20.0/2659.5;
                                                        }
                                                    }
                                                } else {
                                                    return 38.0/3428.6;
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 1362.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.361699998379f ) {
                                            return 77.0/2948.4;
                                        } else {
                                            if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                                return 29.0/2319.7;
                                            } else {
                                                return 51.0/2854.8;
                                            }
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 2089.0f ) {
                                            return 89.0/2561.8;
                                        } else {
                                            return 65.0/2205.7;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 19.5f ) {
                                return 172.0/2970.8;
                            } else {
                                return 191.0/2427.5;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.used_for_uip_creation <= 7.5f ) {
                        if ( rdb0_last_touched_diff <= 2802.5f ) {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 12.5f ) {
                                if ( cl->stats.glue <= 7.5f ) {
                                    return 223.0/3082.7;
                                } else {
                                    return 249.0/2547.5;
                                }
                            } else {
                                return 98.0/2256.6;
                            }
                        } else {
                            if ( cl->stats.size_rel <= 0.540318846703f ) {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 8.5f ) {
                                    return 400.0/3011.5;
                                } else {
                                    return 272.0/3023.7;
                                }
                            } else {
                                return 499.0/3094.9;
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 1563.5f ) {
                            if ( cl->stats.num_overlap_literals <= 63.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.08483530581f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 11.5f ) {
                                        return 77.0/2215.9;
                                    } else {
                                        if ( cl->stats.dump_number <= 6.5f ) {
                                            return 60.0/2285.1;
                                        } else {
                                            return 54.0/3532.4;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 24.5f ) {
                                        return 53.0/3119.3;
                                    } else {
                                        return 17.0/3290.2;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.386810600758f ) {
                                    return 79.0/1931.0;
                                } else {
                                    return 72.0/2549.6;
                                }
                            }
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 19.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 52.5f ) {
                                    return 184.0/3276.0;
                                } else {
                                    return 195.0/2517.0;
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.488129973412f ) {
                                    return 98.0/1872.0;
                                } else {
                                    return 107.0/2659.5;
                                }
                            }
                        }
                    }
                }
            }
        }
    } else {
        if ( cl->stats.antec_num_total_lits_rel <= 0.373553335667f ) {
            if ( cl->size() <= 9.5f ) {
                if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                    if ( cl->stats.rdb1_act_ranking_top_10 <= 4.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 36221.5f ) {
                            if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                                    if ( rdb0_last_touched_diff <= 23064.5f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 15.5f ) {
                                            return 319.0/1477.3;
                                        } else {
                                            return 395.0/1446.7;
                                        }
                                    } else {
                                        return 806.0/2104.0;
                                    }
                                } else {
                                    return 516.0/1109.0;
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 18292.0f ) {
                                    return 378.0/1353.1;
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 7.5f ) {
                                        return 694.0/1778.4;
                                    } else {
                                        return 591.0/1121.2;
                                    }
                                }
                            }
                        } else {
                            if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                if ( cl->size() <= 4.5f ) {
                                    return 837.0/1707.2;
                                } else {
                                    if ( rdb0_last_touched_diff <= 87133.0f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.431122988462f ) {
                                            return 714.0/787.5;
                                        } else {
                                            return 1071.0/1839.4;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.457738101482f ) {
                                            return 789.0/736.6;
                                        } else {
                                            return 839.0/990.9;
                                        }
                                    }
                                }
                            } else {
                                return 939.0/1129.3;
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 120630.0f ) {
                            if ( cl->stats.glue <= 5.5f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.134406000376f ) {
                                    if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                        return 722.0/1062.2;
                                    } else {
                                        return 719.0/1206.6;
                                    }
                                } else {
                                    return 666.0/893.3;
                                }
                            } else {
                                return 1062.0/1222.9;
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 151570.5f ) {
                                if ( rdb0_act_ranking_top_10 <= 7.5f ) {
                                    return 675.0/761.0;
                                } else {
                                    return 838.0/675.5;
                                }
                            } else {
                                if ( cl->stats.dump_number <= 79.5f ) {
                                    if ( rdb0_last_touched_diff <= 411487.5f ) {
                                        if ( cl->stats.glue_rel_long <= 0.659257054329f ) {
                                            if ( cl->stats.glue_rel_queue <= 0.498546004295f ) {
                                                if ( cl->stats.num_total_lits_antecedents <= 14.5f ) {
                                                    return 879.0/667.4;
                                                } else {
                                                    return 1164.0/732.5;
                                                }
                                            } else {
                                                return 779.0/647.1;
                                            }
                                        } else {
                                            return 830.0/476.1;
                                        }
                                    } else {
                                        return 880.0/311.3;
                                    }
                                } else {
                                    return 754.0/736.6;
                                }
                            }
                        }
                    }
                } else {
                    return 292.0/1892.3;
                }
            } else {
                if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                    if ( rdb0_last_touched_diff <= 34201.5f ) {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 4.5f ) {
                            if ( cl->stats.dump_number <= 2.5f ) {
                                return 1361.0/1283.9;
                            } else {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.0493827164173f ) {
                                    return 558.0/1363.3;
                                } else {
                                    return 748.0/1133.4;
                                }
                            }
                        } else {
                            return 351.0/2000.2;
                        }
                    } else {
                        if ( cl->stats.glue_rel_long <= 0.659927845001f ) {
                            return 732.0/706.1;
                        } else {
                            return 839.0/575.8;
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                        if ( rdb0_last_touched_diff <= 94577.0f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.257231414318f ) {
                                if ( cl->stats.glue_rel_long <= 0.692869603634f ) {
                                    if ( rdb0_last_touched_diff <= 40104.0f ) {
                                        return 971.0/1768.2;
                                    } else {
                                        if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                            if ( cl->stats.size_rel <= 0.445802628994f ) {
                                                return 806.0/840.4;
                                            } else {
                                                return 794.0/720.3;
                                            }
                                        } else {
                                            return 922.0/1318.5;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 9.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0810774713755f ) {
                                            return 1416.0/787.5;
                                        } else {
                                            return 1069.0/826.1;
                                        }
                                    } else {
                                        return 874.0/1056.1;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 59.5f ) {
                                    if ( rdb0_last_touched_diff <= 45558.0f ) {
                                        return 1140.0/1088.6;
                                    } else {
                                        if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                            return 1246.0/809.8;
                                        } else {
                                            return 1316.0/636.9;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.133214920759f ) {
                                        if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                            return 990.0/282.8;
                                        } else {
                                            return 1013.0/181.1;
                                        }
                                    } else {
                                        if ( cl->stats.size_rel <= 0.509081721306f ) {
                                            return 1132.0/522.9;
                                        } else {
                                            return 1274.0/374.4;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 156.5f ) {
                                if ( cl->size() <= 12.5f ) {
                                    if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                        if ( cl->stats.glue_rel_long <= 0.640580058098f ) {
                                            return 1259.0/940.1;
                                        } else {
                                            return 971.0/506.7;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.745126664639f ) {
                                            if ( cl->stats.glue_rel_queue <= 0.522761940956f ) {
                                                return 880.0/449.7;
                                            } else {
                                                return 1019.0/461.9;
                                            }
                                        } else {
                                            return 875.0/317.4;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.713521242142f ) {
                                        if ( cl->stats.glue_rel_long <= 0.329126596451f ) {
                                            return 1056.0/759.0;
                                        } else {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.0493827164173f ) {
                                                if ( cl->stats.size_rel <= 0.351452678442f ) {
                                                    return 760.0/512.8;
                                                } else {
                                                    if ( cl->stats.antec_num_total_lits_rel <= 0.200474411249f ) {
                                                        return 883.0/329.6;
                                                    } else {
                                                        return 962.0/500.6;
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.num_overlap_literals_rel <= 0.13478679955f ) {
                                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 6.5f ) {
                                                        return 1267.0/466.0;
                                                    } else {
                                                        return 1136.0/329.6;
                                                    }
                                                } else {
                                                    return 1628.0/697.9;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 162205.5f ) {
                                            if ( cl->stats.dump_number <= 15.5f ) {
                                                if ( cl->stats.glue_rel_long <= 0.889813303947f ) {
                                                    return 882.0/327.6;
                                                } else {
                                                    return 1512.0/309.3;
                                                }
                                            } else {
                                                return 954.0/590.1;
                                            }
                                        } else {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.102286189795f ) {
                                                if ( cl->stats.num_antecedents_rel <= 0.15381243825f ) {
                                                    if ( cl->stats.num_overlap_literals <= 8.5f ) {
                                                        return 914.0/262.5;
                                                    } else {
                                                        return 1060.0/187.2;
                                                    }
                                                } else {
                                                    return 1062.0/398.8;
                                                }
                                            } else {
                                                if ( cl->stats.num_total_lits_antecedents <= 51.5f ) {
                                                    return 1766.0/459.9;
                                                } else {
                                                    if ( cl->stats.glue_rel_queue <= 0.951765775681f ) {
                                                        return 995.0/146.5;
                                                    } else {
                                                        return 1039.0/173.0;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.972447395325f ) {
                                    return 956.0/189.2;
                                } else {
                                    return 1315.0/142.4;
                                }
                            }
                        }
                    } else {
                        return 727.0/972.6;
                    }
                }
            }
        } else {
            if ( cl->stats.glue_rel_queue <= 0.899420976639f ) {
                if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                    if ( rdb0_last_touched_diff <= 61325.5f ) {
                        if ( cl->stats.size_rel <= 0.836256027222f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 12428.5f ) {
                                return 487.0/1137.4;
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 93.5f ) {
                                    return 528.0/1090.6;
                                } else {
                                    return 782.0/824.1;
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.75720012188f ) {
                                return 718.0/693.9;
                            } else {
                                return 815.0/606.4;
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_queue <= 0.692174613476f ) {
                            return 1422.0/948.2;
                        } else {
                            if ( rdb0_last_touched_diff <= 174404.0f ) {
                                return 1638.0/744.7;
                            } else {
                                return 899.0/297.1;
                            }
                        }
                    }
                } else {
                    if ( cl->size() <= 10.5f ) {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 1.07708334923f ) {
                            return 689.0/801.7;
                        } else {
                            return 774.0/586.0;
                        }
                    } else {
                        if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.0807242244482f ) {
                                return 919.0/618.6;
                            } else {
                                if ( rdb0_last_touched_diff <= 49255.5f ) {
                                    return 955.0/549.4;
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 13.2210655212f ) {
                                        if ( cl->stats.glue_rel_long <= 0.555963873863f ) {
                                            return 839.0/337.8;
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 188051.0f ) {
                                                if ( cl->size() <= 21.5f ) {
                                                    return 1230.0/508.7;
                                                } else {
                                                    return 1402.0/400.9;
                                                }
                                            } else {
                                                if ( cl->stats.antecedents_glue_long_reds_var <= 2.82653069496f ) {
                                                    return 1026.0/166.9;
                                                } else {
                                                    return 913.0/248.2;
                                                }
                                            }
                                        }
                                    } else {
                                        return 1284.0/280.8;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.493366539478f ) {
                                return 1015.0/354.1;
                            } else {
                                if ( cl->stats.num_overlap_literals <= 106.5f ) {
                                    return 1585.0/227.9;
                                } else {
                                    return 940.0/234.0;
                                }
                            }
                        }
                    }
                }
            } else {
                if ( rdb0_last_touched_diff <= 62459.5f ) {
                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.252970486879f ) {
                        if ( cl->stats.num_overlap_literals <= 29.5f ) {
                            return 627.0/783.4;
                        } else {
                            return 753.0/647.1;
                        }
                    } else {
                        if ( cl->stats.size_rel <= 0.951233029366f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 189.5f ) {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                                    return 1270.0/1129.3;
                                } else {
                                    return 1007.0/394.7;
                                }
                            } else {
                                if ( cl->stats.dump_number <= 4.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 1.26502156258f ) {
                                        return 1653.0/189.2;
                                    } else {
                                        return 1587.0/291.0;
                                    }
                                } else {
                                    return 1109.0/303.2;
                                }
                            }
                        } else {
                            if ( cl->size() <= 141.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.339907437563f ) {
                                    return 831.0/415.1;
                                } else {
                                    if ( cl->stats.glue <= 14.5f ) {
                                        if ( cl->stats.dump_number <= 3.5f ) {
                                            return 1141.0/258.4;
                                        } else {
                                            return 1146.0/459.9;
                                        }
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 198.5f ) {
                                            return 1164.0/240.1;
                                        } else {
                                            return 1937.0/211.6;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 416.0f ) {
                                    return 1127.0/130.2;
                                } else {
                                    return 1517.0/89.5;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->size() <= 11.5f ) {
                        return 1425.0/620.6;
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 13.2003078461f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.0293590389192f ) {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 7.5f ) {
                                    return 909.0/362.2;
                                } else {
                                    return 1213.0/264.5;
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                    if ( cl->stats.glue_rel_long <= 1.35834980011f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.426996290684f ) {
                                            return 1770.0/484.3;
                                        } else {
                                            if ( rdb0_last_touched_diff <= 117726.0f ) {
                                                if ( cl->stats.glue_rel_queue <= 1.03288006783f ) {
                                                    return 1080.0/335.7;
                                                } else {
                                                    return 1647.0/327.6;
                                                }
                                            } else {
                                                if ( cl->stats.rdb1_act_ranking_top_10 <= 6.5f ) {
                                                    if ( cl->stats.dump_number <= 27.5f ) {
                                                        if ( cl->stats.antecedents_glue_long_reds_var <= 4.3851852417f ) {
                                                            return 1084.0/126.2;
                                                        } else {
                                                            return 1257.0/107.8;
                                                        }
                                                    } else {
                                                        return 1156.0/219.8;
                                                    }
                                                } else {
                                                    if ( rdb0_last_touched_diff <= 255957.5f ) {
                                                        return 955.0/189.2;
                                                    } else {
                                                        return 983.0/213.7;
                                                    }
                                                }
                                            }
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 155355.0f ) {
                                            return 1137.0/166.9;
                                        } else {
                                            return 1117.0/83.4;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 1.35214614868f ) {
                                        if ( cl->stats.num_overlap_literals <= 63.5f ) {
                                            return 1143.0/107.8;
                                        } else {
                                            return 1468.0/207.5;
                                        }
                                    } else {
                                        return 1504.0/79.4;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals <= 476.5f ) {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 9.5f ) {
                                    if ( cl->stats.glue_rel_long <= 1.15839338303f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 1.66324043274f ) {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 21.8958320618f ) {
                                                return 1632.0/288.9;
                                            } else {
                                                if ( cl->stats.num_overlap_literals <= 121.5f ) {
                                                    return 1368.0/189.2;
                                                } else {
                                                    return 1613.0/111.9;
                                                }
                                            }
                                        } else {
                                            return 956.0/195.3;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 1.16545724869f ) {
                                            return 1440.0/197.4;
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.345417559147f ) {
                                                return 1049.0/150.6;
                                            } else {
                                                if ( cl->stats.num_overlap_literals_rel <= 2.26608014107f ) {
                                                    if ( rdb0_act_ranking_top_10 <= 7.5f ) {
                                                        if ( cl->stats.num_overlap_literals_rel <= 1.06311297417f ) {
                                                            return 1580.0/164.8;
                                                        } else {
                                                            return 1410.0/83.4;
                                                        }
                                                    } else {
                                                        return 1498.0/71.2;
                                                    }
                                                } else {
                                                    return 1001.0/111.9;
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    return 1949.0/118.0;
                                }
                            } else {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 8.5f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 132.8175354f ) {
                                        if ( cl->stats.glue_rel_long <= 1.15029478073f ) {
                                            return 984.0/111.9;
                                        } else {
                                            return 1353.0/28.5;
                                        }
                                    } else {
                                        return 1309.0/164.8;
                                    }
                                } else {
                                    return 1091.0/40.7;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

static double estimator_should_keep_short_conf2_cluster0_3(
    const CMSat::Clause* cl
    , const uint64_t sumConflicts
    , const uint32_t rdb0_last_touched_diff
    , const uint32_t rdb0_act_ranking
    , const uint32_t rdb0_act_ranking_top_10
) {

    double rdb_rel_used_for_uip_creation = 0;
    if (cl->stats.used_for_uip_creation > cl->stats.rdb1_used_for_uip_creation) {
        rdb_rel_used_for_uip_creation = 1.0;
    } else {
        rdb_rel_used_for_uip_creation = 0.0;
    }

    double rdb0_avg_confl;
    if (cl->stats.sum_delta_confl_uip1_used == 0) {
        rdb0_avg_confl = 0;
    } else {
        rdb0_avg_confl = ((double)cl->stats.sum_uip1_used)/((double)cl->stats.sum_delta_confl_uip1_used);
    }

    double rdb0_used_per_confl;
    if (sumConflicts-cl->stats.introduced_at_conflict == 0) {
        rdb0_used_per_confl = 0;
    } else {
        rdb0_used_per_confl = ((double)cl->stats.sum_uip1_used)/((double)sumConflicts-(double)cl->stats.introduced_at_conflict);
    }
    if ( rdb0_act_ranking_top_10 <= 2.5f ) {
        if ( cl->stats.rdb1_last_touched_diff <= 11047.5f ) {
            if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                if ( cl->stats.glue_rel_queue <= 0.915989398956f ) {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 29.5f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 17.5f ) {
                                if ( cl->size() <= 6.5f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0553929954767f ) {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                            return 614.0/1975.8;
                                        } else {
                                            return 333.0/1589.2;
                                        }
                                    } else {
                                        return 287.0/1605.4;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.48974686861f ) {
                                        return 334.0/1371.4;
                                    } else {
                                        return 417.0/1471.1;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.580682635307f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 5006.0f ) {
                                        return 618.0/1768.2;
                                    } else {
                                        return 503.0/2006.3;
                                    }
                                } else {
                                    return 841.0/2242.3;
                                }
                            }
                        } else {
                            if ( cl->stats.size_rel <= 0.801975846291f ) {
                                if ( cl->size() <= 9.5f ) {
                                    return 695.0/2293.2;
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 39.5f ) {
                                        return 669.0/1701.1;
                                    } else {
                                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 5636.5f ) {
                                                return 721.0/1680.7;
                                            } else {
                                                return 621.0/1137.4;
                                            }
                                        } else {
                                            return 868.0/1241.2;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 81.5f ) {
                                    return 684.0/1157.8;
                                } else {
                                    return 858.0/1066.2;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.948750019073f ) {
                            if ( rdb0_last_touched_diff <= 13844.0f ) {
                                if ( cl->size() <= 7.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 1859.5f ) {
                                        return 304.0/2213.8;
                                    } else {
                                        return 185.0/1813.0;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.559980511665f ) {
                                        return 456.0/2494.6;
                                    } else {
                                        return 251.0/1839.4;
                                    }
                                }
                            } else {
                                if ( cl->size() <= 11.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 15.5f ) {
                                        return 327.0/1867.9;
                                    } else {
                                        return 433.0/2057.2;
                                    }
                                } else {
                                    return 364.0/1387.7;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 10.5f ) {
                                if ( cl->stats.dump_number <= 4.5f ) {
                                    return 397.0/1367.4;
                                } else {
                                    return 359.0/1436.6;
                                }
                            } else {
                                return 293.0/1621.7;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.antecedents_glue_long_reds_var <= 7.47164821625f ) {
                        if ( cl->stats.num_overlap_literals_rel <= 0.204913288355f ) {
                            if ( cl->stats.glue_rel_long <= 1.00076878071f ) {
                                return 467.0/1534.2;
                            } else {
                                return 655.0/1241.2;
                            }
                        } else {
                            if ( cl->stats.size_rel <= 0.726503968239f ) {
                                return 621.0/1074.4;
                            } else {
                                return 1016.0/1135.4;
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_long <= 1.01743650436f ) {
                            return 717.0/700.0;
                        } else {
                            if ( cl->stats.num_overlap_literals_rel <= 0.62494301796f ) {
                                return 799.0/482.2;
                            } else {
                                return 1533.0/500.6;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.antecedents_glue_long_reds_var <= 0.0108312200755f ) {
                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                        if ( rdb0_last_touched_diff <= 2748.5f ) {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 9.5f ) {
                                if ( cl->stats.size_rel <= 0.762003421783f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                        if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                            return 254.0/1731.6;
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 0.261051297188f ) {
                                                return 193.0/2232.2;
                                            } else {
                                                if ( rdb0_last_touched_diff <= 1057.5f ) {
                                                    if ( cl->stats.glue_rel_long <= 0.539902448654f ) {
                                                        if ( rdb0_last_touched_diff <= 378.5f ) {
                                                            return 88.0/1957.5;
                                                        } else {
                                                            return 111.0/1837.4;
                                                        }
                                                    } else {
                                                        return 79.0/1967.6;
                                                    }
                                                } else {
                                                    if ( cl->stats.used_for_uip_creation <= 5.5f ) {
                                                        return 189.0/1707.2;
                                                    } else {
                                                        return 144.0/2270.8;
                                                    }
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.0717305243015f ) {
                                            if ( cl->stats.used_for_uip_creation <= 8.5f ) {
                                                return 245.0/2569.9;
                                            } else {
                                                return 173.0/3670.7;
                                            }
                                        } else {
                                            if ( cl->stats.dump_number <= 4.5f ) {
                                                if ( rdb0_last_touched_diff <= 673.5f ) {
                                                    return 75.0/1910.7;
                                                } else {
                                                    return 133.0/2142.6;
                                                }
                                            } else {
                                                if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                                    return 206.0/2702.2;
                                                } else {
                                                    if ( cl->stats.glue_rel_long <= 0.296807527542f ) {
                                                        return 115.0/2578.1;
                                                    } else {
                                                        if ( cl->stats.size_rel <= 0.307548850775f ) {
                                                            if ( cl->stats.used_for_uip_creation <= 11.5f ) {
                                                                return 62.0/2433.6;
                                                            } else {
                                                                return 37.0/2077.5;
                                                            }
                                                        } else {
                                                            return 111.0/3418.4;
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    return 246.0/2409.2;
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 697.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0864480882883f ) {
                                        if ( cl->stats.glue <= 5.5f ) {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.00739719765261f ) {
                                                return 15.0/2472.3;
                                            } else {
                                                if ( cl->stats.rdb1_used_for_uip_creation <= 107.5f ) {
                                                    if ( cl->stats.rdb1_used_for_uip_creation <= 28.5f ) {
                                                        if ( rdb0_last_touched_diff <= 671.5f ) {
                                                            return 35.0/2169.1;
                                                        } else {
                                                            return 63.0/1975.8;
                                                        }
                                                    } else {
                                                        if ( cl->stats.num_overlap_literals_rel <= 0.0211773328483f ) {
                                                            return 44.0/2706.3;
                                                        } else {
                                                            return 16.0/2248.4;
                                                        }
                                                    }
                                                } else {
                                                    return 71.0/2069.4;
                                                }
                                            }
                                        } else {
                                            return 112.0/3672.8;
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 517.5f ) {
                                            if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                                return 24.0/3630.1;
                                            } else {
                                                return 9.0/4085.8;
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 0.484380543232f ) {
                                                return 29.0/2824.3;
                                            } else {
                                                return 53.0/2838.5;
                                            }
                                        }
                                    }
                                } else {
                                    if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                        if ( cl->stats.size_rel <= 0.148398607969f ) {
                                            if ( cl->stats.used_for_uip_creation <= 15.5f ) {
                                                return 190.0/3068.5;
                                            } else {
                                                return 49.0/2399.0;
                                            }
                                        } else {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.126776784658f ) {
                                                if ( cl->size() <= 6.5f ) {
                                                    return 68.0/2576.0;
                                                } else {
                                                    return 95.0/2608.6;
                                                }
                                            } else {
                                                if ( cl->stats.glue_rel_queue <= 0.55330145359f ) {
                                                    return 29.0/2163.0;
                                                } else {
                                                    return 57.0/2014.4;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 16.5f ) {
                                            if ( cl->stats.num_overlap_literals <= 6.5f ) {
                                                return 53.0/2466.2;
                                            } else {
                                                return 75.0/1975.8;
                                            }
                                        } else {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.0982637405396f ) {
                                                return 73.0/3825.4;
                                            } else {
                                                return 19.0/2453.9;
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                if ( cl->stats.size_rel <= 0.47363960743f ) {
                                    if ( cl->stats.glue_rel_long <= 0.330239474773f ) {
                                        return 471.0/3119.3;
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0593973137438f ) {
                                            return 246.0/1642.1;
                                        } else {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0826742574573f ) {
                                                return 309.0/3135.6;
                                            } else {
                                                return 237.0/1684.8;
                                            }
                                        }
                                    }
                                } else {
                                    return 332.0/1581.0;
                                }
                            } else {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                                    if ( rdb0_last_touched_diff <= 6125.5f ) {
                                        if ( cl->size() <= 6.5f ) {
                                            return 223.0/2586.2;
                                        } else {
                                            return 320.0/2545.5;
                                        }
                                    } else {
                                        return 251.0/1770.3;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 9.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.504686594009f ) {
                                            if ( rdb0_last_touched_diff <= 4958.5f ) {
                                                return 158.0/2926.0;
                                            } else {
                                                return 157.0/1749.9;
                                            }
                                        } else {
                                            return 219.0/2329.8;
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0316392481327f ) {
                                            if ( cl->stats.dump_number <= 12.5f ) {
                                                return 146.0/2490.6;
                                            } else {
                                                return 165.0/2366.5;
                                            }
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 15.5f ) {
                                                return 74.0/2525.2;
                                            } else {
                                                if ( cl->stats.num_antecedents_rel <= 0.200846165419f ) {
                                                    return 112.0/1969.7;
                                                } else {
                                                    return 98.0/2140.6;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.antec_num_total_lits_rel <= 0.149746179581f ) {
                            if ( cl->stats.used_for_uip_creation <= 5.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0325648523867f ) {
                                    return 315.0/1878.1;
                                } else {
                                    return 258.0/1851.7;
                                }
                            } else {
                                return 200.0/3066.4;
                            }
                        } else {
                            return 260.0/2995.2;
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_last_touched_diff <= 1548.5f ) {
                        if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                            if ( cl->stats.num_overlap_literals <= 41.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.107932649553f ) {
                                    return 283.0/1784.5;
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.218042105436f ) {
                                        return 227.0/2026.6;
                                    } else {
                                        return 222.0/1709.2;
                                    }
                                }
                            } else {
                                return 425.0/2024.6;
                            }
                        } else {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 6.5f ) {
                                if ( rdb0_last_touched_diff <= 1693.5f ) {
                                    return 137.0/2895.5;
                                } else {
                                    return 299.0/2407.1;
                                }
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 17.5f ) {
                                    if ( cl->size() <= 8.5f ) {
                                        return 55.0/2032.7;
                                    } else {
                                        if ( cl->stats.dump_number <= 11.5f ) {
                                            return 189.0/2744.9;
                                        } else {
                                            return 103.0/2010.4;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.145957380533f ) {
                                        if ( cl->stats.num_overlap_literals <= 16.5f ) {
                                            return 77.0/3538.5;
                                        } else {
                                            return 69.0/1994.1;
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 89.5f ) {
                                            if ( rdb0_last_touched_diff <= 491.5f ) {
                                                return 8.0/2146.7;
                                            } else {
                                                return 54.0/2309.5;
                                            }
                                        } else {
                                            return 49.0/2014.4;
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                            if ( rdb0_last_touched_diff <= 2797.5f ) {
                                if ( cl->size() <= 72.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.776271641254f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.410789489746f ) {
                                            if ( cl->stats.num_total_lits_antecedents <= 30.5f ) {
                                                if ( rdb0_last_touched_diff <= 547.5f ) {
                                                    return 102.0/2480.4;
                                                } else {
                                                    if ( cl->stats.rdb1_used_for_uip_creation <= 4.5f ) {
                                                        return 191.0/2020.5;
                                                    } else {
                                                        return 88.0/1941.2;
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.rdb1_last_touched_diff <= 5921.0f ) {
                                                    if ( cl->stats.used_for_uip_creation <= 7.5f ) {
                                                        return 179.0/1707.2;
                                                    } else {
                                                        return 103.0/2645.2;
                                                    }
                                                } else {
                                                    return 179.0/1884.2;
                                                }
                                            }
                                        } else {
                                            return 164.0/1827.2;
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 770.0f ) {
                                            return 154.0/2279.0;
                                        } else {
                                            return 324.0/2810.0;
                                        }
                                    }
                                } else {
                                    return 220.0/1743.8;
                                }
                            } else {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 4.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.646876811981f ) {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                            return 339.0/1967.6;
                                        } else {
                                            return 282.0/2281.0;
                                        }
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 79.5f ) {
                                            return 454.0/2376.6;
                                        } else {
                                            return 468.0/1513.9;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 29.5f ) {
                                        return 155.0/1985.9;
                                    } else {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 9.5f ) {
                                            return 237.0/1774.3;
                                        } else {
                                            return 173.0/1737.7;
                                        }
                                    }
                                }
                            }
                        } else {
                            return 547.0/2565.9;
                        }
                    }
                }
            }
        } else {
            if ( rdb0_last_touched_diff <= 9812.0f ) {
                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                    if ( cl->size() <= 12.5f ) {
                        if ( rdb0_last_touched_diff <= 2228.5f ) {
                            if ( cl->stats.glue_rel_long <= 0.762494504452f ) {
                                if ( rdb0_last_touched_diff <= 302.5f ) {
                                    if ( cl->stats.size_rel <= 0.20722463727f ) {
                                        return 159.0/2075.5;
                                    } else {
                                        return 110.0/2606.6;
                                    }
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0595391206443f ) {
                                            return 408.0/1845.5;
                                        } else {
                                            return 338.0/2136.5;
                                        }
                                    } else {
                                        if ( cl->stats.dump_number <= 10.5f ) {
                                            return 134.0/2266.7;
                                        } else {
                                            if ( cl->stats.used_for_uip_creation <= 9.5f ) {
                                                return 211.0/1900.5;
                                            } else {
                                                return 110.0/1849.6;
                                            }
                                        }
                                    }
                                }
                            } else {
                                return 238.0/1605.4;
                            }
                        } else {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 4.5f ) {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.102997668087f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 21636.0f ) {
                                            return 444.0/3292.3;
                                        } else {
                                            return 277.0/1690.9;
                                        }
                                    } else {
                                        return 404.0/1859.8;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0286559909582f ) {
                                        return 389.0/1404.0;
                                    } else {
                                        if ( rdb0_last_touched_diff <= 3496.5f ) {
                                            return 270.0/1815.0;
                                        } else {
                                            if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                                                return 443.0/1489.5;
                                            } else {
                                                return 498.0/2677.8;
                                            }
                                        }
                                    }
                                }
                            } else {
                                return 416.0/1275.8;
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 16406.5f ) {
                                return 440.0/2901.6;
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 80.5f ) {
                                    return 492.0/2814.1;
                                } else {
                                    return 401.0/1520.0;
                                }
                            }
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 4.5f ) {
                                    if ( rdb0_last_touched_diff <= 2020.0f ) {
                                        return 413.0/1768.2;
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 36033.5f ) {
                                            return 588.0/1456.9;
                                        } else {
                                            return 607.0/1066.2;
                                        }
                                    }
                                } else {
                                    return 765.0/1343.0;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 8.5f ) {
                                    return 143.0/1827.2;
                                } else {
                                    if ( rdb0_last_touched_diff <= 1301.5f ) {
                                        return 152.0/2830.4;
                                    } else {
                                        return 415.0/2083.6;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_total_lits_antecedents <= 44.5f ) {
                        return 547.0/1715.3;
                    } else {
                        return 624.0/1133.4;
                    }
                }
            } else {
                if ( cl->stats.glue_rel_queue <= 1.00112748146f ) {
                    if ( cl->stats.size_rel <= 0.489488959312f ) {
                        if ( rdb0_last_touched_diff <= 59213.0f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 18594.5f ) {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                    if ( cl->size() <= 9.5f ) {
                                        if ( cl->stats.dump_number <= 7.5f ) {
                                            return 461.0/1798.7;
                                        } else {
                                            if ( cl->stats.num_overlap_literals <= 6.5f ) {
                                                return 543.0/2036.8;
                                            } else {
                                                return 443.0/1326.7;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.glue <= 7.5f ) {
                                            return 504.0/1151.7;
                                        } else {
                                            return 509.0/1064.2;
                                        }
                                    }
                                } else {
                                    return 724.0/1416.2;
                                }
                            } else {
                                if ( cl->stats.glue <= 6.5f ) {
                                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0435502976179f ) {
                                            return 383.0/1267.7;
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 20.5f ) {
                                                return 448.0/1440.6;
                                            } else {
                                                return 667.0/1430.5;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 32898.0f ) {
                                            if ( cl->stats.num_antecedents_rel <= 0.13794901967f ) {
                                                return 633.0/1780.4;
                                            } else {
                                                return 756.0/1550.5;
                                            }
                                        } else {
                                            return 962.0/1709.2;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 6.5f ) {
                                        return 1148.0/1393.8;
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.596950531006f ) {
                                            return 730.0/1275.8;
                                        } else {
                                            return 596.0/1373.5;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.glue <= 7.5f ) {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                    return 719.0/1467.1;
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.482692062855f ) {
                                        return 913.0/1322.6;
                                    } else {
                                        return 759.0/889.2;
                                    }
                                }
                            } else {
                                return 1323.0/1147.6;
                            }
                        }
                    } else {
                        if ( cl->stats.num_overlap_literals <= 27.5f ) {
                            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.glue_rel_queue <= 0.703775048256f ) {
                                    if ( cl->size() <= 15.5f ) {
                                        return 481.0/1355.2;
                                    } else {
                                        return 660.0/915.7;
                                    }
                                } else {
                                    return 1056.0/1593.2;
                                }
                            } else {
                                if ( cl->size() <= 26.5f ) {
                                    if ( cl->size() <= 15.5f ) {
                                        if ( cl->stats.glue <= 7.5f ) {
                                            return 863.0/1324.6;
                                        } else {
                                            return 694.0/887.2;
                                        }
                                    } else {
                                        return 966.0/1007.2;
                                    }
                                } else {
                                    return 811.0/539.2;
                                }
                            }
                        } else {
                            if ( cl->stats.dump_number <= 13.5f ) {
                                if ( cl->stats.glue_rel_long <= 0.887966811657f ) {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                        if ( cl->stats.glue <= 11.5f ) {
                                            return 1313.0/1456.9;
                                        } else {
                                            return 753.0/659.3;
                                        }
                                    } else {
                                        return 1367.0/877.0;
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 47351.5f ) {
                                        return 885.0/612.5;
                                    } else {
                                        return 914.0/246.2;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                    return 824.0/1204.6;
                                } else {
                                    return 807.0/592.1;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_antecedents_rel <= 0.388341009617f ) {
                        if ( rdb0_last_touched_diff <= 60814.0f ) {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.245969861746f ) {
                                return 752.0/1062.2;
                            } else {
                                return 1206.0/881.1;
                            }
                        } else {
                            return 1023.0/319.5;
                        }
                    } else {
                        if ( cl->stats.glue <= 11.5f ) {
                            return 966.0/765.1;
                        } else {
                            if ( cl->stats.num_overlap_literals_rel <= 1.13005566597f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.636562108994f ) {
                                    return 963.0/266.6;
                                } else {
                                    return 1047.0/248.2;
                                }
                            } else {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                    return 1278.0/254.3;
                                } else {
                                    return 1342.0/122.1;
                                }
                            }
                        }
                    }
                }
            }
        }
    } else {
        if ( cl->size() <= 11.5f ) {
            if ( cl->stats.num_total_lits_antecedents <= 21.5f ) {
                if ( cl->stats.glue <= 4.5f ) {
                    if ( rdb0_last_touched_diff <= 43752.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 7147.5f ) {
                            return 382.0/2425.5;
                        } else {
                            if ( cl->stats.num_overlap_literals_rel <= 0.0323116630316f ) {
                                return 597.0/1432.5;
                            } else {
                                return 701.0/2122.3;
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 123242.5f ) {
                            if ( cl->stats.num_antecedents_rel <= 0.139655455947f ) {
                                if ( cl->stats.dump_number <= 11.5f ) {
                                    return 569.0/854.6;
                                } else {
                                    return 895.0/1540.3;
                                }
                            } else {
                                return 1150.0/1633.9;
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 266904.0f ) {
                                if ( cl->size() <= 4.5f ) {
                                    return 703.0/893.3;
                                } else {
                                    return 826.0/634.9;
                                }
                            } else {
                                return 964.0/700.0;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_antecedents_rel <= 0.149124622345f ) {
                        if ( cl->stats.antec_num_total_lits_rel <= 0.0470711141825f ) {
                            return 1174.0/1434.5;
                        } else {
                            if ( cl->stats.num_overlap_literals_rel <= 0.0451994091272f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.0732392668724f ) {
                                    return 636.0/954.3;
                                } else {
                                    return 793.0/944.1;
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 103231.0f ) {
                                    return 766.0/2242.3;
                                } else {
                                    return 698.0/659.3;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.dump_number <= 13.5f ) {
                            if ( cl->stats.dump_number <= 6.5f ) {
                                return 515.0/1349.1;
                            } else {
                                return 652.0/948.2;
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals_rel <= 0.0998665094376f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 151998.0f ) {
                                    return 713.0/773.2;
                                } else {
                                    return 858.0/392.7;
                                }
                            } else {
                                return 676.0/756.9;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                    if ( cl->stats.antec_num_total_lits_rel <= 0.732316017151f ) {
                        if ( cl->stats.dump_number <= 11.5f ) {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                if ( cl->size() <= 8.5f ) {
                                    return 843.0/1699.0;
                                } else {
                                    return 1087.0/1566.8;
                                }
                            } else {
                                return 371.0/1444.7;
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.0943827182055f ) {
                                return 754.0/1220.9;
                            } else {
                                return 777.0/954.3;
                            }
                        }
                    } else {
                        return 944.0/954.3;
                    }
                } else {
                    if ( rdb0_last_touched_diff <= 136761.5f ) {
                        if ( cl->stats.num_overlap_literals <= 69.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 27662.5f ) {
                                return 566.0/1104.9;
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 7.5f ) {
                                    if ( cl->stats.size_rel <= 0.207133173943f ) {
                                        return 631.0/1056.1;
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.354209721088f ) {
                                            return 1518.0/1068.3;
                                        } else {
                                            return 659.0/763.0;
                                        }
                                    }
                                } else {
                                    return 806.0/653.2;
                                }
                            }
                        } else {
                            return 1379.0/736.6;
                        }
                    } else {
                        if ( cl->stats.glue <= 5.5f ) {
                            if ( cl->stats.size_rel <= 0.210799992085f ) {
                                return 804.0/846.5;
                            } else {
                                return 1014.0/590.1;
                            }
                        } else {
                            if ( cl->stats.dump_number <= 23.5f ) {
                                return 1735.0/557.5;
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 7.5f ) {
                                    return 1193.0/740.7;
                                } else {
                                    return 1201.0/543.3;
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.num_overlap_literals <= 40.5f ) {
                if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.0355029590428f ) {
                        if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                            if ( cl->stats.size_rel <= 0.333256959915f ) {
                                return 540.0/1125.2;
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.190596938133f ) {
                                    return 755.0/826.1;
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 12.5f ) {
                                        return 599.0/1066.2;
                                    } else {
                                        return 640.0/858.7;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.271404325962f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 45992.5f ) {
                                    return 742.0/1296.2;
                                } else {
                                    return 845.0/531.1;
                                }
                            } else {
                                return 864.0/685.7;
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 29464.5f ) {
                            if ( cl->stats.dump_number <= 3.5f ) {
                                if ( cl->stats.size_rel <= 0.796450078487f ) {
                                    return 924.0/1100.8;
                                } else {
                                    return 967.0/439.5;
                                }
                            } else {
                                return 1055.0/1672.6;
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.69276791811f ) {
                                if ( cl->stats.size_rel <= 0.685713648796f ) {
                                    return 1338.0/974.7;
                                } else {
                                    return 825.0/490.4;
                                }
                            } else {
                                if ( cl->size() <= 15.5f ) {
                                    return 1158.0/651.1;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 95818.5f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.238848909736f ) {
                                            return 804.0/427.3;
                                        } else {
                                            return 1057.0/315.4;
                                        }
                                    } else {
                                        return 1690.0/423.2;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.0355029590428f ) {
                        if ( cl->stats.size_rel <= 0.572599053383f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 113276.0f ) {
                                if ( cl->stats.glue_rel_long <= 0.678486168385f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 47793.0f ) {
                                        return 522.0/1060.1;
                                    } else {
                                        return 594.0/956.3;
                                    }
                                } else {
                                    return 818.0/693.9;
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.617012023926f ) {
                                    return 1578.0/842.4;
                                } else {
                                    return 1045.0/376.4;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 54888.5f ) {
                                return 1086.0/1180.2;
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 62.5f ) {
                                    if ( cl->stats.dump_number <= 35.5f ) {
                                        if ( rdb0_act_ranking_top_10 <= 7.5f ) {
                                            return 1086.0/360.2;
                                        } else {
                                            return 1050.0/221.8;
                                        }
                                    } else {
                                        return 871.0/307.3;
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 25.5f ) {
                                        return 1293.0/600.3;
                                    } else {
                                        return 946.0/331.7;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 38277.0f ) {
                            return 1572.0/968.6;
                        } else {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.392485231161f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 132818.0f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.897953569889f ) {
                                        if ( cl->stats.glue_rel_long <= 0.5671916008f ) {
                                            return 849.0/502.6;
                                        } else {
                                            return 1693.0/702.0;
                                        }
                                    } else {
                                        return 1638.0/380.5;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 9.5f ) {
                                        if ( cl->size() <= 23.5f ) {
                                            if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                                if ( cl->stats.glue_rel_queue <= 0.627483844757f ) {
                                                    return 848.0/358.1;
                                                } else {
                                                    if ( cl->stats.size_rel <= 0.672525882721f ) {
                                                        return 965.0/221.8;
                                                    } else {
                                                        return 910.0/333.7;
                                                    }
                                                }
                                            } else {
                                                return 952.0/225.9;
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 0.815737247467f ) {
                                                return 917.0/252.3;
                                            } else {
                                                return 960.0/187.2;
                                            }
                                        }
                                    } else {
                                        return 1312.0/248.2;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 8.5f ) {
                                    if ( cl->stats.size_rel <= 0.860494494438f ) {
                                        return 888.0/311.3;
                                    } else {
                                        if ( cl->stats.glue <= 9.5f ) {
                                            return 938.0/288.9;
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 61.5f ) {
                                                return 1094.0/142.4;
                                            } else {
                                                return 1614.0/321.5;
                                            }
                                        }
                                    }
                                } else {
                                    return 1941.0/225.9;
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.antecedents_glue_long_reds_var <= 9.40284729004f ) {
                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.00119759794325f ) {
                        if ( cl->stats.size_rel <= 0.480196833611f ) {
                            return 991.0/1290.1;
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 70399.0f ) {
                                return 738.0/567.7;
                            } else {
                                return 1352.0/370.3;
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 47324.5f ) {
                            if ( cl->size() <= 117.5f ) {
                                if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                    if ( cl->stats.dump_number <= 4.5f ) {
                                        return 1360.0/750.8;
                                    } else {
                                        return 629.0/809.8;
                                    }
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 1.10729908943f ) {
                                        if ( cl->stats.glue_rel_long <= 0.84654545784f ) {
                                            return 865.0/708.1;
                                        } else {
                                            return 1364.0/543.3;
                                        }
                                    } else {
                                        return 1185.0/370.3;
                                    }
                                }
                            } else {
                                return 1129.0/234.0;
                            }
                        } else {
                            if ( cl->stats.size_rel <= 1.03984498978f ) {
                                if ( cl->stats.num_overlap_literals <= 384.5f ) {
                                    if ( rdb0_last_touched_diff <= 161642.5f ) {
                                        if ( cl->stats.dump_number <= 15.5f ) {
                                            if ( cl->stats.glue_rel_queue <= 0.843645572662f ) {
                                                return 1435.0/529.0;
                                            } else {
                                                if ( cl->stats.glue <= 10.5f ) {
                                                    return 929.0/295.0;
                                                } else {
                                                    return 1456.0/234.0;
                                                }
                                            }
                                        } else {
                                            return 819.0/539.2;
                                        }
                                    } else {
                                        if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                            if ( cl->stats.num_overlap_literals <= 122.5f ) {
                                                if ( rdb0_last_touched_diff <= 236014.5f ) {
                                                    return 968.0/272.7;
                                                } else {
                                                    return 1435.0/286.9;
                                                }
                                            } else {
                                                return 913.0/274.7;
                                            }
                                        } else {
                                            return 1249.0/185.2;
                                        }
                                    }
                                } else {
                                    return 1326.0/189.2;
                                }
                            } else {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 8.5f ) {
                                    if ( cl->size() <= 101.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.694200217724f ) {
                                            return 1620.0/407.0;
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 1.4818829298f ) {
                                                if ( cl->stats.glue_rel_queue <= 0.9567745924f ) {
                                                    return 877.0/260.5;
                                                } else {
                                                    return 1593.0/223.8;
                                                }
                                            } else {
                                                return 1389.0/187.2;
                                            }
                                        }
                                    } else {
                                        return 1047.0/95.6;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 99.5f ) {
                                        return 1011.0/93.6;
                                    } else {
                                        return 1006.0/111.9;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.glue_rel_queue <= 0.960242211819f ) {
                        if ( cl->stats.glue_rel_long <= 0.814019322395f ) {
                            if ( cl->size() <= 57.5f ) {
                                return 1641.0/813.9;
                            } else {
                                return 1159.0/337.8;
                            }
                        } else {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                                return 964.0/284.9;
                            } else {
                                if ( rdb0_last_touched_diff <= 96890.0f ) {
                                    return 1052.0/270.6;
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.714818358421f ) {
                                        return 1268.0/146.5;
                                    } else {
                                        return 981.0/185.2;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 39711.5f ) {
                            if ( cl->stats.glue_rel_queue <= 1.30040717125f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.843563377857f ) {
                                    return 1187.0/347.9;
                                } else {
                                    if ( cl->stats.glue <= 21.5f ) {
                                        return 981.0/250.3;
                                    } else {
                                        return 1141.0/168.9;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 1.127140522f ) {
                                    return 1073.0/158.7;
                                } else {
                                    return 1331.0/124.1;
                                }
                            }
                        } else {
                            if ( cl->stats.glue <= 14.5f ) {
                                if ( rdb0_act_ranking_top_10 <= 7.5f ) {
                                    if ( cl->stats.glue_rel_long <= 1.18846845627f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 1.03381276131f ) {
                                            return 892.0/248.2;
                                        } else {
                                            return 1093.0/238.1;
                                        }
                                    } else {
                                        return 1146.0/154.6;
                                    }
                                } else {
                                    return 1436.0/107.8;
                                }
                            } else {
                                if ( cl->stats.size_rel <= 1.24655365944f ) {
                                    if ( cl->stats.dump_number <= 35.5f ) {
                                        if ( cl->stats.glue <= 43.5f ) {
                                            if ( cl->stats.size_rel <= 0.929232478142f ) {
                                                if ( cl->stats.glue_rel_queue <= 1.04993438721f ) {
                                                    return 1085.0/118.0;
                                                } else {
                                                    if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                                        return 1581.0/57.0;
                                                    } else {
                                                        return 1966.0/138.4;
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.antecedents_glue_long_reds_var <= 25.5938396454f ) {
                                                    return 968.0/111.9;
                                                } else {
                                                    return 1476.0/132.3;
                                                }
                                            }
                                        } else {
                                            if ( cl->size() <= 91.5f ) {
                                                return 1186.0/242.1;
                                            } else {
                                                return 1457.0/197.4;
                                            }
                                        }
                                    } else {
                                        return 967.0/219.8;
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 5.5f ) {
                                        return 1370.0/38.7;
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 698.0f ) {
                                            if ( rdb0_last_touched_diff <= 114088.0f ) {
                                                return 1769.0/225.9;
                                            } else {
                                                if ( cl->stats.num_overlap_literals <= 263.5f ) {
                                                    if ( cl->stats.glue <= 23.5f ) {
                                                        if ( cl->stats.num_antecedents_rel <= 0.83657848835f ) {
                                                            return 1009.0/69.2;
                                                        } else {
                                                            return 1002.0/81.4;
                                                        }
                                                    } else {
                                                        return 1703.0/52.9;
                                                    }
                                                } else {
                                                    return 1041.0/107.8;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 1.21233725548f ) {
                                                return 1005.0/75.3;
                                            } else {
                                                return 1687.0/28.5;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

static double estimator_should_keep_short_conf2_cluster0_4(
    const CMSat::Clause* cl
    , const uint64_t sumConflicts
    , const uint32_t rdb0_last_touched_diff
    , const uint32_t rdb0_act_ranking
    , const uint32_t rdb0_act_ranking_top_10
) {

    double rdb_rel_used_for_uip_creation = 0;
    if (cl->stats.used_for_uip_creation > cl->stats.rdb1_used_for_uip_creation) {
        rdb_rel_used_for_uip_creation = 1.0;
    } else {
        rdb_rel_used_for_uip_creation = 0.0;
    }

    double rdb0_avg_confl;
    if (cl->stats.sum_delta_confl_uip1_used == 0) {
        rdb0_avg_confl = 0;
    } else {
        rdb0_avg_confl = ((double)cl->stats.sum_uip1_used)/((double)cl->stats.sum_delta_confl_uip1_used);
    }

    double rdb0_used_per_confl;
    if (sumConflicts-cl->stats.introduced_at_conflict == 0) {
        rdb0_used_per_confl = 0;
    } else {
        rdb0_used_per_confl = ((double)cl->stats.sum_uip1_used)/((double)sumConflicts-(double)cl->stats.introduced_at_conflict);
    }
    if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
        if ( cl->stats.used_for_uip_creation <= 0.5f ) {
            if ( cl->size() <= 11.5f ) {
                if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                    if ( cl->stats.rdb1_last_touched_diff <= 33513.5f ) {
                        if ( cl->size() <= 9.5f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.size_rel <= 0.0669206976891f ) {
                                    return 316.0/1428.4;
                                } else {
                                    if ( cl->size() <= 6.5f ) {
                                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                            if ( cl->stats.size_rel <= 0.173882842064f ) {
                                                return 471.0/1538.3;
                                            } else {
                                                return 289.0/1670.6;
                                            }
                                        } else {
                                            return 555.0/1737.7;
                                        }
                                    } else {
                                        if ( cl->stats.size_rel <= 0.211509495974f ) {
                                            return 545.0/1302.3;
                                        } else {
                                            if ( cl->stats.dump_number <= 23.5f ) {
                                                return 704.0/2405.1;
                                            } else {
                                                return 515.0/1275.8;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 1.02771604061f ) {
                                    if ( cl->stats.num_overlap_literals <= 11.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0658964961767f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 20338.0f ) {
                                                return 518.0/1072.3;
                                            } else {
                                                return 487.0/1257.5;
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 21599.5f ) {
                                                return 759.0/2478.4;
                                            } else {
                                                return 738.0/1939.1;
                                            }
                                        }
                                    } else {
                                        return 821.0/1874.0;
                                    }
                                } else {
                                    return 649.0/1135.4;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                if ( rdb0_last_touched_diff <= 27824.5f ) {
                                    return 531.0/1469.1;
                                } else {
                                    return 579.0/1049.9;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 12.5f ) {
                                    return 579.0/1206.6;
                                } else {
                                    return 760.0/964.5;
                                }
                            }
                        }
                    } else {
                        if ( cl->size() <= 7.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 69023.5f ) {
                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                    return 416.0/1277.8;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 41236.0f ) {
                                        return 604.0/1430.5;
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.113861739635f ) {
                                            return 539.0/1133.4;
                                        } else {
                                            return 958.0/1550.5;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                                    return 799.0/1275.8;
                                } else {
                                    return 839.0/1015.4;
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 109310.5f ) {
                                if ( cl->stats.glue <= 8.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.237921401858f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.194867551327f ) {
                                            if ( cl->stats.glue_rel_queue <= 0.544520497322f ) {
                                                return 937.0/1332.8;
                                            } else {
                                                return 725.0/1454.9;
                                            }
                                        } else {
                                            return 738.0/915.7;
                                        }
                                    } else {
                                        return 1092.0/1257.5;
                                    }
                                } else {
                                    return 785.0/651.1;
                                }
                            } else {
                                return 893.0/663.3;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_total_lits_antecedents <= 21.5f ) {
                        if ( cl->stats.size_rel <= 0.160464137793f ) {
                            if ( cl->stats.dump_number <= 16.5f ) {
                                return 1046.0/2032.7;
                            } else {
                                if ( rdb0_last_touched_diff <= 276871.0f ) {
                                    if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                        return 779.0/1068.3;
                                    } else {
                                        return 704.0/689.8;
                                    }
                                } else {
                                    return 996.0/661.3;
                                }
                            }
                        } else {
                            if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 100625.0f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.145194783807f ) {
                                        return 541.0/1035.7;
                                    } else {
                                        return 665.0/903.4;
                                    }
                                } else {
                                    return 910.0/708.1;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.102509781718f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.121723547578f ) {
                                        if ( cl->stats.dump_number <= 21.5f ) {
                                            return 678.0/771.2;
                                        } else {
                                            return 1292.0/891.2;
                                        }
                                    } else {
                                        return 1167.0/695.9;
                                    }
                                } else {
                                    return 854.0/968.6;
                                }
                            }
                        }
                    } else {
                        if ( cl->size() <= 7.5f ) {
                            if ( cl->stats.num_overlap_literals_rel <= 0.150896817446f ) {
                                return 1187.0/1066.2;
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 111533.5f ) {
                                    return 716.0/1086.6;
                                } else {
                                    return 760.0/846.5;
                                }
                            }
                        } else {
                            if ( cl->stats.num_antecedents_rel <= 0.912519872189f ) {
                                if ( cl->stats.glue_rel_long <= 0.726686477661f ) {
                                    if ( rdb0_last_touched_diff <= 154918.5f ) {
                                        if ( cl->stats.num_overlap_literals <= 17.5f ) {
                                            return 1253.0/984.8;
                                        } else {
                                            return 801.0/838.3;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.568082928658f ) {
                                            return 1365.0/596.2;
                                        } else {
                                            return 866.0/514.8;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 5.5f ) {
                                        return 1168.0/789.5;
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.917899250984f ) {
                                            return 862.0/386.6;
                                        } else {
                                            return 884.0/333.7;
                                        }
                                    }
                                }
                            } else {
                                return 1468.0/549.4;
                            }
                        }
                    }
                }
            } else {
                if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                    if ( cl->stats.glue_rel_queue <= 0.959896802902f ) {
                        if ( rdb0_last_touched_diff <= 57005.5f ) {
                            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.471952080727f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 93.5f ) {
                                        if ( rdb0_last_touched_diff <= 29456.0f ) {
                                            if ( cl->size() <= 16.5f ) {
                                                return 495.0/1247.3;
                                            } else {
                                                return 715.0/1406.0;
                                            }
                                        } else {
                                            if ( cl->stats.dump_number <= 5.5f ) {
                                                return 673.0/832.2;
                                            } else {
                                                return 852.0/1650.2;
                                            }
                                        }
                                    } else {
                                        return 808.0/913.6;
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 4.5f ) {
                                        return 731.0/683.7;
                                    } else {
                                        return 633.0/1056.1;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.717531740665f ) {
                                    if ( cl->stats.num_overlap_literals <= 51.5f ) {
                                        if ( rdb0_last_touched_diff <= 34250.5f ) {
                                            if ( cl->stats.dump_number <= 4.5f ) {
                                                return 703.0/950.2;
                                            } else {
                                                return 893.0/1796.7;
                                            }
                                        } else {
                                            if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                                                return 988.0/1283.9;
                                            } else {
                                                return 722.0/754.9;
                                            }
                                        }
                                    } else {
                                        return 857.0/858.7;
                                    }
                                } else {
                                    if ( cl->stats.glue <= 11.5f ) {
                                        if ( cl->stats.num_overlap_literals <= 17.5f ) {
                                            return 761.0/1007.2;
                                        } else {
                                            if ( cl->size() <= 17.5f ) {
                                                return 704.0/832.2;
                                            } else {
                                                return 957.0/722.3;
                                            }
                                        }
                                    } else {
                                        return 1503.0/988.9;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.208567112684f ) {
                                    return 1091.0/1373.5;
                                } else {
                                    return 1111.0/649.1;
                                }
                            } else {
                                if ( cl->stats.size_rel <= 0.877191245556f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 123.5f ) {
                                        if ( cl->size() <= 23.5f ) {
                                            if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                                                return 1266.0/1037.7;
                                            } else {
                                                return 1534.0/999.1;
                                            }
                                        } else {
                                            return 849.0/466.0;
                                        }
                                    } else {
                                        return 896.0/327.6;
                                    }
                                } else {
                                    if ( cl->size() <= 33.5f ) {
                                        return 862.0/451.7;
                                    } else {
                                        return 1137.0/372.4;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue <= 11.5f ) {
                            if ( cl->stats.num_overlap_literals <= 34.5f ) {
                                return 1222.0/1369.4;
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 36541.5f ) {
                                    return 1101.0/783.4;
                                } else {
                                    return 867.0/382.5;
                                }
                            }
                        } else {
                            if ( cl->stats.dump_number <= 13.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 4.24244880676f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.379264950752f ) {
                                        return 841.0/569.7;
                                    } else {
                                        return 1053.0/319.5;
                                    }
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.441474705935f ) {
                                        return 1453.0/482.2;
                                    } else {
                                        if ( cl->stats.glue <= 26.5f ) {
                                            if ( cl->stats.num_overlap_literals <= 361.5f ) {
                                                if ( cl->stats.glue_rel_queue <= 1.08212780952f ) {
                                                    return 1142.0/382.5;
                                                } else {
                                                    if ( cl->stats.num_antecedents_rel <= 0.881508409977f ) {
                                                        return 1369.0/215.7;
                                                    } else {
                                                        return 1267.0/286.9;
                                                    }
                                                }
                                            } else {
                                                return 1046.0/146.5;
                                            }
                                        } else {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 181.183197021f ) {
                                                return 1982.0/95.6;
                                            } else {
                                                return 973.0/111.9;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->size() <= 57.5f ) {
                                    return 1276.0/864.8;
                                } else {
                                    return 756.0/461.9;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_overlap_literals <= 37.5f ) {
                        if ( cl->stats.size_rel <= 0.697504401207f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 87839.0f ) {
                                if ( cl->stats.glue_rel_queue <= 0.638265073299f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.0694444477558f ) {
                                        if ( cl->stats.dump_number <= 7.5f ) {
                                            return 683.0/1078.4;
                                        } else {
                                            return 594.0/1161.9;
                                        }
                                    } else {
                                        return 967.0/793.6;
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.0824332684278f ) {
                                        return 835.0/329.6;
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.425000011921f ) {
                                            return 1032.0/1127.3;
                                        } else {
                                            return 1110.0/622.6;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.0355029590428f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 288959.0f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 145806.5f ) {
                                            return 1262.0/962.5;
                                        } else {
                                            return 1622.0/838.3;
                                        }
                                    } else {
                                        return 1602.0/472.1;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.820008337498f ) {
                                        if ( cl->stats.rdb1_act_ranking_top_10 <= 5.5f ) {
                                            return 1434.0/695.9;
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.176997244358f ) {
                                                return 1230.0/358.1;
                                            } else {
                                                return 1111.0/443.6;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 3.56790113449f ) {
                                            return 868.0/270.6;
                                        } else {
                                            return 963.0/166.9;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 60807.0f ) {
                                if ( cl->stats.glue_rel_queue <= 0.849156737328f ) {
                                    return 1455.0/1078.4;
                                } else {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 4.5f ) {
                                        return 1359.0/740.7;
                                    } else {
                                        return 1202.0/486.3;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.840894341469f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 147825.0f ) {
                                        if ( cl->size() <= 24.5f ) {
                                            return 1371.0/502.6;
                                        } else {
                                            return 980.0/520.9;
                                        }
                                    } else {
                                        if ( cl->stats.glue <= 10.5f ) {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.9921875f ) {
                                                return 1347.0/278.8;
                                            } else {
                                                return 1223.0/376.4;
                                            }
                                        } else {
                                            return 1265.0/431.4;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.553072094917f ) {
                                        if ( rdb0_act_ranking_top_10 <= 9.5f ) {
                                            if ( cl->stats.glue_rel_queue <= 1.23303031921f ) {
                                                if ( cl->stats.dump_number <= 41.5f ) {
                                                    if ( cl->stats.size_rel <= 1.21438169479f ) {
                                                        if ( cl->stats.num_overlap_literals_rel <= 0.12442792207f ) {
                                                            return 901.0/335.7;
                                                        } else {
                                                            if ( cl->stats.rdb1_last_touched_diff <= 147648.0f ) {
                                                                return 915.0/242.1;
                                                            } else {
                                                                return 917.0/201.4;
                                                            }
                                                        }
                                                    } else {
                                                        return 1322.0/272.7;
                                                    }
                                                } else {
                                                    return 885.0/358.1;
                                                }
                                            } else {
                                                return 1454.0/258.4;
                                            }
                                        } else {
                                            return 1121.0/136.3;
                                        }
                                    } else {
                                        return 1694.0/205.5;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue <= 11.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 51477.5f ) {
                                if ( cl->stats.dump_number <= 4.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 158.5f ) {
                                        return 867.0/429.3;
                                    } else {
                                        return 933.0/258.4;
                                    }
                                } else {
                                    return 1182.0/911.6;
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.585690498352f ) {
                                    if ( cl->stats.glue_rel_long <= 0.442106306553f ) {
                                        return 818.0/516.8;
                                    } else {
                                        return 939.0/325.6;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 8.5f ) {
                                        if ( cl->stats.size_rel <= 1.12378752232f ) {
                                            if ( rdb0_last_touched_diff <= 211073.5f ) {
                                                if ( cl->stats.num_total_lits_antecedents <= 115.5f ) {
                                                    return 1529.0/645.0;
                                                } else {
                                                    if ( cl->stats.antec_num_total_lits_rel <= 0.99265897274f ) {
                                                        return 967.0/225.9;
                                                    } else {
                                                        return 867.0/337.8;
                                                    }
                                                }
                                            } else {
                                                return 1834.0/433.4;
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 154932.5f ) {
                                                return 1253.0/311.3;
                                            } else {
                                                return 1153.0/158.7;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.849298417568f ) {
                                            return 1287.0/175.0;
                                        } else {
                                            return 1045.0/101.7;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.882446169853f ) {
                                if ( rdb0_last_touched_diff <= 65196.5f ) {
                                    return 1294.0/669.4;
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 13.9511260986f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.32464659214f ) {
                                            return 849.0/362.2;
                                        } else {
                                            return 1692.0/496.5;
                                        }
                                    } else {
                                        if ( cl->stats.dump_number <= 17.5f ) {
                                            return 950.0/170.9;
                                        } else {
                                            return 969.0/221.8;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.640844464302f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 7.14795684814f ) {
                                        if ( cl->stats.size_rel <= 0.863763689995f ) {
                                            return 1402.0/457.8;
                                        } else {
                                            return 1047.0/221.8;
                                        }
                                    } else {
                                        if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                            if ( rdb0_last_touched_diff <= 130091.0f ) {
                                                if ( cl->stats.glue_rel_long <= 1.2089240551f ) {
                                                    return 1457.0/305.2;
                                                } else {
                                                    return 1448.0/181.1;
                                                }
                                            } else {
                                                return 1937.0/201.4;
                                            }
                                        } else {
                                            return 1319.0/109.9;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 52.5f ) {
                                        if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                            if ( rdb0_last_touched_diff <= 32542.0f ) {
                                                return 1573.0/270.6;
                                            } else {
                                                if ( cl->stats.num_overlap_literals <= 171.5f ) {
                                                    if ( cl->stats.glue <= 17.5f ) {
                                                        if ( cl->stats.num_antecedents_rel <= 0.614926099777f ) {
                                                            return 923.0/195.3;
                                                        } else {
                                                            if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                                                if ( cl->stats.glue_rel_queue <= 1.10716819763f ) {
                                                                    return 1051.0/197.4;
                                                                } else {
                                                                    return 1789.0/179.1;
                                                                }
                                                            } else {
                                                                return 1270.0/236.0;
                                                            }
                                                        }
                                                    } else {
                                                        if ( cl->stats.num_overlap_literals_rel <= 1.13520979881f ) {
                                                            if ( cl->stats.num_antecedents_rel <= 0.625658273697f ) {
                                                                return 1181.0/95.6;
                                                            } else {
                                                                return 1003.0/111.9;
                                                            }
                                                        } else {
                                                            return 937.0/138.4;
                                                        }
                                                    }
                                                } else {
                                                    if ( cl->stats.size_rel <= 2.23362183571f ) {
                                                        if ( cl->stats.glue_rel_long <= 1.15681195259f ) {
                                                            if ( cl->stats.num_total_lits_antecedents <= 1202.5f ) {
                                                                if ( cl->stats.size_rel <= 0.758501827717f ) {
                                                                    return 1220.0/107.8;
                                                                } else {
                                                                    return 1926.0/254.3;
                                                                }
                                                            } else {
                                                                return 930.0/183.1;
                                                            }
                                                        } else {
                                                            if ( cl->stats.glue_rel_long <= 1.43996024132f ) {
                                                                if ( cl->stats.glue_rel_long <= 1.26011884212f ) {
                                                                    return 1521.0/97.7;
                                                                } else {
                                                                    return 1475.0/175.0;
                                                                }
                                                            } else {
                                                                if ( cl->stats.rdb1_act_ranking_top_10 <= 5.5f ) {
                                                                    return 1156.0/34.6;
                                                                } else {
                                                                    return 1065.0/105.8;
                                                                }
                                                            }
                                                        }
                                                    } else {
                                                        return 1873.0/103.8;
                                                    }
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.size_rel <= 1.27589678764f ) {
                                                if ( cl->stats.glue_rel_queue <= 1.12959194183f ) {
                                                    return 935.0/124.1;
                                                } else {
                                                    return 1771.0/136.3;
                                                }
                                            } else {
                                                if ( cl->stats.num_overlap_literals_rel <= 1.11151862144f ) {
                                                    return 1339.0/85.5;
                                                } else {
                                                    if ( cl->stats.num_overlap_literals <= 256.5f ) {
                                                        return 992.0/30.5;
                                                    } else {
                                                        return 1174.0/20.3;
                                                    }
                                                }
                                            }
                                        }
                                    } else {
                                        return 1491.0/333.7;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if ( rdb0_last_touched_diff <= 2228.5f ) {
                if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                    if ( cl->stats.rdb1_last_touched_diff <= 18395.0f ) {
                        if ( cl->stats.num_overlap_literals_rel <= 0.0691547393799f ) {
                            return 306.0/2348.1;
                        } else {
                            return 365.0/2543.5;
                        }
                    } else {
                        if ( cl->stats.rdb1_act_ranking_top_10 <= 4.5f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 52.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.105148926377f ) {
                                    return 316.0/1467.1;
                                } else {
                                    return 486.0/3151.9;
                                }
                            } else {
                                return 424.0/1621.7;
                            }
                        } else {
                            return 523.0/1837.4;
                        }
                    }
                } else {
                    if ( rdb0_last_touched_diff <= 781.5f ) {
                        if ( cl->stats.num_overlap_literals_rel <= 0.195501714945f ) {
                            if ( cl->stats.dump_number <= 23.5f ) {
                                if ( cl->stats.used_for_uip_creation <= 12.5f ) {
                                    return 122.0/2502.8;
                                } else {
                                    return 81.0/3330.9;
                                }
                            } else {
                                return 144.0/2785.6;
                            }
                        } else {
                            return 116.0/2016.5;
                        }
                    } else {
                        if ( cl->stats.antec_num_total_lits_rel <= 0.0828745514154f ) {
                            return 225.0/2034.8;
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 8.5f ) {
                                return 187.0/2034.8;
                            } else {
                                return 149.0/2667.6;
                            }
                        }
                    }
                }
            } else {
                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                    if ( cl->stats.glue <= 7.5f ) {
                        if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                            if ( cl->size() <= 9.5f ) {
                                if ( cl->stats.glue_rel_long <= 0.445700883865f ) {
                                    return 572.0/1929.0;
                                } else {
                                    return 494.0/2069.4;
                                }
                            } else {
                                return 675.0/1684.8;
                            }
                        } else {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.208762139082f ) {
                                if ( cl->stats.used_for_uip_creation <= 7.5f ) {
                                    if ( cl->stats.size_rel <= 0.203278928995f ) {
                                        return 613.0/2822.2;
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.149508431554f ) {
                                            return 354.0/2006.3;
                                        } else {
                                            return 248.0/1749.9;
                                        }
                                    }
                                } else {
                                    return 291.0/2726.6;
                                }
                            } else {
                                return 498.0/2590.3;
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                            if ( cl->stats.glue_rel_queue <= 0.832851648331f ) {
                                return 526.0/2541.4;
                            } else {
                                return 387.0/1467.1;
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.95356798172f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 45384.0f ) {
                                    return 584.0/2175.2;
                                } else {
                                    return 640.0/1599.3;
                                }
                            } else {
                                return 620.0/1399.9;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_overlap_literals <= 30.5f ) {
                        if ( cl->size() <= 9.5f ) {
                            return 455.0/1662.4;
                        } else {
                            return 593.0/1430.5;
                        }
                    } else {
                        return 606.0/879.0;
                    }
                }
            }
        }
    } else {
        if ( cl->stats.antec_num_total_lits_rel <= 0.54119759798f ) {
            if ( cl->stats.glue_rel_queue <= 0.709977030754f ) {
                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                    if ( cl->stats.rdb1_last_touched_diff <= 3066.5f ) {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 5.5f ) {
                            if ( cl->stats.glue_rel_long <= 0.579817593098f ) {
                                if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.67708337307f ) {
                                        if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                                            if ( cl->stats.glue_rel_long <= 0.378592133522f ) {
                                                return 349.0/1705.1;
                                            } else {
                                                return 445.0/1735.7;
                                            }
                                        } else {
                                            if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                                                return 352.0/2354.2;
                                            } else {
                                                return 270.0/2958.6;
                                            }
                                        }
                                    } else {
                                        return 446.0/1566.8;
                                    }
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 10.5f ) {
                                        if ( cl->size() <= 9.5f ) {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                                return 193.0/1766.2;
                                            } else {
                                                if ( cl->stats.num_overlap_literals_rel <= 0.0433121882379f ) {
                                                    return 126.0/1827.2;
                                                } else {
                                                    return 105.0/1941.2;
                                                }
                                            }
                                        } else {
                                            return 293.0/1981.9;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.287583112717f ) {
                                            return 128.0/1837.4;
                                        } else {
                                            if ( rdb0_last_touched_diff <= 1237.0f ) {
                                                return 86.0/2598.4;
                                            } else {
                                                return 99.0/1867.9;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                                    return 454.0/1654.3;
                                } else {
                                    return 302.0/3416.4;
                                }
                            }
                        } else {
                            if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                if ( rdb0_last_touched_diff <= 7691.0f ) {
                                    if ( cl->size() <= 8.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0292629636824f ) {
                                            if ( rdb0_last_touched_diff <= 1492.0f ) {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.0260248184204f ) {
                                                    if ( cl->stats.used_for_uip_creation <= 29.5f ) {
                                                        return 66.0/2189.4;
                                                    } else {
                                                        return 33.0/2154.8;
                                                    }
                                                } else {
                                                    return 89.0/2718.5;
                                                }
                                            } else {
                                                if ( cl->stats.rdb1_used_for_uip_creation <= 22.5f ) {
                                                    return 142.0/1870.0;
                                                } else {
                                                    return 95.0/1992.1;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.used_for_uip_creation <= 10.5f ) {
                                                if ( cl->stats.size_rel <= 0.171396464109f ) {
                                                    return 119.0/2645.2;
                                                } else {
                                                    if ( cl->stats.glue_rel_long <= 0.374841839075f ) {
                                                        return 43.0/2132.5;
                                                    } else {
                                                        return 116.0/3550.7;
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.glue_rel_queue <= 0.431406706572f ) {
                                                    if ( cl->stats.size_rel <= 0.207073330879f ) {
                                                        return 65.0/3037.9;
                                                    } else {
                                                        return 16.0/2055.1;
                                                    }
                                                } else {
                                                    return 24.0/2590.3;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.used_for_uip_creation <= 10.5f ) {
                                            if ( rdb0_last_touched_diff <= 1173.5f ) {
                                                return 119.0/2681.8;
                                            } else {
                                                if ( cl->size() <= 14.5f ) {
                                                    return 143.0/2014.4;
                                                } else {
                                                    return 213.0/2342.0;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 0.266270041466f ) {
                                                return 74.0/2158.9;
                                            } else {
                                                if ( rdb0_last_touched_diff <= 489.5f ) {
                                                    return 26.0/3408.3;
                                                } else {
                                                    if ( cl->stats.size_rel <= 0.339094638824f ) {
                                                        return 59.0/2144.7;
                                                    } else {
                                                        return 46.0/2110.1;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 18.5f ) {
                                        return 355.0/3180.4;
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 967.5f ) {
                                            return 225.0/1804.9;
                                        } else {
                                            return 265.0/1823.2;
                                        }
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 1898.5f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0244808774441f ) {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 11.5f ) {
                                            return 91.0/2244.4;
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 22.5f ) {
                                                if ( cl->stats.glue_rel_queue <= 0.319386810064f ) {
                                                    return 61.0/3400.1;
                                                } else {
                                                    return 45.0/1929.0;
                                                }
                                            } else {
                                                return 29.0/2317.6;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.dump_number <= 2.5f ) {
                                            return 63.0/2962.6;
                                        } else {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 10.5f ) {
                                                return 81.0/3469.3;
                                            } else {
                                                if ( rdb0_last_touched_diff <= 729.0f ) {
                                                    if ( cl->stats.rdb1_last_touched_diff <= 114.5f ) {
                                                        return 13.0/2293.2;
                                                    } else {
                                                        if ( cl->stats.num_antecedents_rel <= 0.155199423432f ) {
                                                            return 29.0/2476.3;
                                                        } else {
                                                            return 21.0/3062.3;
                                                        }
                                                    }
                                                } else {
                                                    return 39.0/2635.0;
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 5.5f ) {
                                        return 110.0/2681.8;
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 534.0f ) {
                                            return 118.0/1870.0;
                                        } else {
                                            return 99.0/1929.0;
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.num_overlap_literals <= 4.5f ) {
                            if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                                if ( cl->stats.glue_rel_long <= 0.380793094635f ) {
                                    return 331.0/1473.2;
                                } else {
                                    return 337.0/1577.0;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0316993780434f ) {
                                    if ( cl->stats.used_for_uip_creation <= 6.5f ) {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                            return 279.0/1857.8;
                                        } else {
                                            return 212.0/2376.6;
                                        }
                                    } else {
                                        if ( cl->stats.size_rel <= 0.138538390398f ) {
                                            return 134.0/2059.2;
                                        } else {
                                            return 79.0/2173.1;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 30.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.407719492912f ) {
                                            return 76.0/2065.3;
                                        } else {
                                            return 98.0/1931.0;
                                        }
                                    } else {
                                        return 141.0/2513.0;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.112465277314f ) {
                                if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                                    if ( cl->size() <= 10.5f ) {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                            return 610.0/2169.1;
                                        } else {
                                            return 446.0/2439.7;
                                        }
                                    } else {
                                        return 469.0/1613.6;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 7764.5f ) {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                            return 202.0/1770.3;
                                        } else {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 7.5f ) {
                                                if ( rdb0_last_touched_diff <= 924.5f ) {
                                                    return 91.0/2065.3;
                                                } else {
                                                    return 275.0/3680.9;
                                                }
                                            } else {
                                                if ( cl->stats.num_antecedents_rel <= 0.104668237269f ) {
                                                    return 100.0/2020.5;
                                                } else {
                                                    return 102.0/3349.3;
                                                }
                                            }
                                        }
                                    } else {
                                        return 239.0/2706.3;
                                    }
                                }
                            } else {
                                if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 6581.5f ) {
                                        if ( cl->size() <= 10.5f ) {
                                            return 385.0/2777.5;
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.169738650322f ) {
                                                return 340.0/1754.0;
                                            } else {
                                                return 404.0/1528.1;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.size_rel <= 0.375184059143f ) {
                                            return 361.0/1719.4;
                                        } else {
                                            return 471.0/1454.9;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.546069145203f ) {
                                        if ( cl->size() <= 10.5f ) {
                                            return 124.0/1967.6;
                                        } else {
                                            return 149.0/1939.1;
                                        }
                                    } else {
                                        return 226.0/2163.0;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                        if ( cl->size() <= 7.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 7139.5f ) {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 5.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.367197602987f ) {
                                        return 507.0/1806.9;
                                    } else {
                                        return 491.0/2230.1;
                                    }
                                } else {
                                    return 274.0/2783.6;
                                }
                            } else {
                                return 526.0/1751.9;
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.180169761181f ) {
                                if ( cl->stats.dump_number <= 2.5f ) {
                                    return 635.0/1509.8;
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.122665092349f ) {
                                        return 446.0/1338.9;
                                    } else {
                                        return 507.0/2307.4;
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 11895.5f ) {
                                    return 464.0/1310.4;
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 17.5f ) {
                                        return 579.0/1463.0;
                                    } else {
                                        return 670.0/1027.6;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 4169.5f ) {
                            if ( rdb0_last_touched_diff <= 6269.5f ) {
                                if ( cl->stats.num_overlap_literals <= 7.5f ) {
                                    return 138.0/2643.2;
                                } else {
                                    return 168.0/2317.6;
                                }
                            } else {
                                return 334.0/2820.2;
                            }
                        } else {
                            return 279.0/1762.1;
                        }
                    }
                }
            } else {
                if ( cl->stats.dump_number <= 1.5f ) {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 112.5f ) {
                            if ( rdb0_last_touched_diff <= 10550.5f ) {
                                return 320.0/1434.5;
                            } else {
                                if ( cl->stats.glue <= 9.5f ) {
                                    return 644.0/1090.6;
                                } else {
                                    return 772.0/498.5;
                                }
                            }
                        } else {
                            return 832.0/402.9;
                        }
                    } else {
                        if ( cl->stats.glue <= 7.5f ) {
                            return 225.0/1676.7;
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                                return 567.0/997.0;
                            } else {
                                return 234.0/2262.7;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_last_touched_diff <= 1594.5f ) {
                        if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                            if ( cl->stats.glue_rel_long <= 0.863490581512f ) {
                                return 354.0/1579.0;
                            } else {
                                return 501.0/1562.7;
                            }
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 9.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 50.5f ) {
                                    return 255.0/3182.4;
                                } else {
                                    return 209.0/1815.0;
                                }
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.207425266504f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 229.5f ) {
                                        return 57.0/2561.8;
                                    } else {
                                        return 142.0/3518.1;
                                    }
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 35.5f ) {
                                        return 36.0/2075.5;
                                    } else {
                                        return 10.0/2287.1;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 25.5f ) {
                                return 623.0/2283.0;
                            } else {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                        return 620.0/1324.6;
                                    } else {
                                        return 853.0/1208.7;
                                    }
                                } else {
                                    return 689.0/2598.4;
                                }
                            }
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                if ( rdb0_last_touched_diff <= 1743.5f ) {
                                    return 250.0/1947.3;
                                } else {
                                    if ( cl->size() <= 10.5f ) {
                                        return 274.0/1876.1;
                                    } else {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                            return 421.0/1395.9;
                                        } else {
                                            return 251.0/1745.8;
                                        }
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 3712.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 33.5f ) {
                                        return 94.0/2655.4;
                                    } else {
                                        if ( rdb0_last_touched_diff <= 780.0f ) {
                                            return 78.0/2163.0;
                                        } else {
                                            return 156.0/2372.6;
                                        }
                                    }
                                } else {
                                    return 177.0/1754.0;
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if ( rdb0_last_touched_diff <= 9818.0f ) {
                if ( cl->stats.dump_number <= 1.5f ) {
                    return 455.0/2112.1;
                } else {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                        if ( cl->stats.glue_rel_queue <= 0.88689494133f ) {
                            return 277.0/1819.1;
                        } else {
                            return 314.0/1546.4;
                        }
                    } else {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 20.5f ) {
                            if ( cl->stats.dump_number <= 10.5f ) {
                                return 205.0/2213.8;
                            } else {
                                return 102.0/2382.7;
                            }
                        } else {
                            return 57.0/2480.4;
                        }
                    }
                }
            } else {
                if ( cl->size() <= 16.5f ) {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                        return 1070.0/1204.6;
                    } else {
                        return 437.0/1680.7;
                    }
                } else {
                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                        if ( cl->stats.glue_rel_long <= 0.961446583271f ) {
                            return 589.0/1375.5;
                        } else {
                            return 1037.0/744.7;
                        }
                    } else {
                        if ( cl->stats.dump_number <= 1.5f ) {
                            if ( cl->stats.glue_rel_long <= 1.02402591705f ) {
                                return 935.0/421.2;
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 1.00800764561f ) {
                                    return 1097.0/144.5;
                                } else {
                                    return 963.0/154.6;
                                }
                            }
                        } else {
                            return 718.0/1027.6;
                        }
                    }
                }
            }
        }
    }
}

static double estimator_should_keep_short_conf2_cluster0_5(
    const CMSat::Clause* cl
    , const uint64_t sumConflicts
    , const uint32_t rdb0_last_touched_diff
    , const uint32_t rdb0_act_ranking
    , const uint32_t rdb0_act_ranking_top_10
) {

    double rdb_rel_used_for_uip_creation = 0;
    if (cl->stats.used_for_uip_creation > cl->stats.rdb1_used_for_uip_creation) {
        rdb_rel_used_for_uip_creation = 1.0;
    } else {
        rdb_rel_used_for_uip_creation = 0.0;
    }

    double rdb0_avg_confl;
    if (cl->stats.sum_delta_confl_uip1_used == 0) {
        rdb0_avg_confl = 0;
    } else {
        rdb0_avg_confl = ((double)cl->stats.sum_uip1_used)/((double)cl->stats.sum_delta_confl_uip1_used);
    }

    double rdb0_used_per_confl;
    if (sumConflicts-cl->stats.introduced_at_conflict == 0) {
        rdb0_used_per_confl = 0;
    } else {
        rdb0_used_per_confl = ((double)cl->stats.sum_uip1_used)/((double)sumConflicts-(double)cl->stats.introduced_at_conflict);
    }
    if ( rdb0_act_ranking_top_10 <= 2.5f ) {
        if ( cl->stats.used_for_uip_creation <= 0.5f ) {
            if ( cl->stats.antecedents_glue_long_reds_var <= 6.42190980911f ) {
                if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                    if ( rdb0_last_touched_diff <= 29503.5f ) {
                        if ( cl->stats.size_rel <= 0.640944898129f ) {
                            if ( cl->size() <= 9.5f ) {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 5090.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.135001063347f ) {
                                            return 648.0/2138.6;
                                        } else {
                                            return 474.0/1355.2;
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 17220.0f ) {
                                            return 312.0/1458.9;
                                        } else {
                                            if ( cl->size() <= 5.5f ) {
                                                if ( cl->stats.glue_rel_queue <= 0.383509457111f ) {
                                                    return 489.0/1918.8;
                                                } else {
                                                    return 309.0/1550.5;
                                                }
                                            } else {
                                                if ( cl->stats.glue_rel_queue <= 0.379147261381f ) {
                                                    return 431.0/1220.9;
                                                } else {
                                                    if ( cl->stats.dump_number <= 6.5f ) {
                                                        return 345.0/1489.5;
                                                    } else {
                                                        if ( cl->stats.glue_rel_queue <= 0.55511534214f ) {
                                                            return 357.0/1402.0;
                                                        } else {
                                                            return 427.0/1233.1;
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 5.5f ) {
                                        if ( cl->size() <= 6.5f ) {
                                            if ( cl->stats.num_antecedents_rel <= 0.153955087066f ) {
                                                return 563.0/2761.2;
                                            } else {
                                                return 300.0/1963.6;
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 0.46829611063f ) {
                                                return 369.0/1383.7;
                                            } else {
                                                return 497.0/2384.8;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.290866881609f ) {
                                            return 318.0/1719.4;
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 3747.5f ) {
                                                return 361.0/3326.9;
                                            } else {
                                                return 343.0/2032.7;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 4.5f ) {
                                    if ( cl->stats.glue <= 11.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 14243.0f ) {
                                            if ( cl->size() <= 11.5f ) {
                                                if ( cl->stats.rdb1_last_touched_diff <= 5294.5f ) {
                                                    return 435.0/1501.7;
                                                } else {
                                                    return 632.0/1745.8;
                                                }
                                            } else {
                                                if ( cl->stats.glue_rel_queue <= 0.823287367821f ) {
                                                    if ( rdb0_last_touched_diff <= 16206.5f ) {
                                                        if ( cl->stats.size_rel <= 0.406443953514f ) {
                                                            return 413.0/1294.1;
                                                        } else {
                                                            return 519.0/1345.0;
                                                        }
                                                    } else {
                                                        return 928.0/2091.8;
                                                    }
                                                } else {
                                                    return 558.0/1009.3;
                                                }
                                            }
                                        } else {
                                            return 807.0/1542.4;
                                        }
                                    } else {
                                        return 742.0/1159.8;
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 13473.5f ) {
                                        return 482.0/2732.7;
                                    } else {
                                        return 500.0/2026.6;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.898784637451f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 3137.0f ) {
                                    return 723.0/2142.6;
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.58278632164f ) {
                                        return 575.0/1347.0;
                                    } else {
                                        if ( cl->stats.size_rel <= 0.785280048847f ) {
                                            return 488.0/1186.3;
                                        } else {
                                            return 1050.0/1591.2;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.572456240654f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.231466561556f ) {
                                        return 567.0/1013.3;
                                    } else {
                                        return 621.0/925.8;
                                    }
                                } else {
                                    return 785.0/618.6;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.antec_num_total_lits_rel <= 0.269249767065f ) {
                            if ( cl->stats.glue <= 6.5f ) {
                                if ( cl->stats.size_rel <= 0.0740931928158f ) {
                                    return 396.0/1389.8;
                                } else {
                                    if ( cl->stats.glue <= 4.5f ) {
                                        return 790.0/2071.4;
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 19.5f ) {
                                            return 575.0/1438.6;
                                        } else {
                                            return 773.0/1408.1;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.glue <= 11.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 26.5f ) {
                                        return 846.0/1560.7;
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 39.5f ) {
                                            return 641.0/944.1;
                                        } else {
                                            return 743.0/887.2;
                                        }
                                    }
                                } else {
                                    return 667.0/767.1;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 37562.0f ) {
                                if ( cl->stats.dump_number <= 5.5f ) {
                                    return 911.0/958.4;
                                } else {
                                    return 600.0/1302.3;
                                }
                            } else {
                                if ( cl->size() <= 19.5f ) {
                                    return 699.0/673.5;
                                } else {
                                    return 814.0/486.3;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.size_rel <= 0.620572507381f ) {
                        if ( cl->stats.num_antecedents_rel <= 0.369678169489f ) {
                            if ( rdb0_last_touched_diff <= 60685.0f ) {
                                if ( cl->stats.dump_number <= 5.5f ) {
                                    return 1036.0/1776.4;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 34523.5f ) {
                                        if ( cl->stats.glue_rel_long <= 0.494974732399f ) {
                                            if ( rdb0_last_touched_diff <= 31831.0f ) {
                                                return 450.0/1119.1;
                                            } else {
                                                return 419.0/1237.1;
                                            }
                                        } else {
                                            return 854.0/1646.1;
                                        }
                                    } else {
                                        return 909.0/1633.9;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue <= 5.5f ) {
                                    return 806.0/1300.2;
                                } else {
                                    return 1389.0/1172.0;
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 47086.0f ) {
                                return 555.0/976.7;
                            } else {
                                return 741.0/608.4;
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 64159.5f ) {
                            if ( cl->stats.num_overlap_literals <= 25.5f ) {
                                return 1166.0/1271.7;
                            } else {
                                return 1503.0/1013.3;
                            }
                        } else {
                            return 1027.0/474.1;
                        }
                    }
                }
            } else {
                if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                    if ( cl->stats.glue_rel_long <= 0.938431859016f ) {
                        if ( cl->stats.glue_rel_long <= 0.832268834114f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 77.5f ) {
                                return 1005.0/2124.3;
                            } else {
                                if ( rdb0_last_touched_diff <= 31001.0f ) {
                                    return 919.0/1823.2;
                                } else {
                                    return 739.0/675.5;
                                }
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals <= 77.5f ) {
                                return 650.0/944.1;
                            } else {
                                return 768.0/677.6;
                            }
                        }
                    } else {
                        if ( cl->stats.dump_number <= 10.5f ) {
                            if ( cl->stats.num_overlap_literals_rel <= 0.435668766499f ) {
                                return 1396.0/897.3;
                            } else {
                                if ( cl->stats.glue_rel_queue <= 1.0499894619f ) {
                                    return 1172.0/636.9;
                                } else {
                                    if ( cl->stats.glue <= 20.5f ) {
                                        return 1496.0/435.4;
                                    } else {
                                        return 1476.0/195.3;
                                    }
                                }
                            }
                        } else {
                            return 604.0/952.3;
                        }
                    }
                } else {
                    if ( cl->stats.glue_rel_queue <= 0.985789299011f ) {
                        if ( cl->stats.glue_rel_long <= 0.757821798325f ) {
                            return 721.0/734.6;
                        } else {
                            return 1161.0/614.5;
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 39726.5f ) {
                            return 1206.0/366.3;
                        } else {
                            return 1876.0/238.1;
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                    if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 43.5f ) {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 2007.5f ) {
                                    return 348.0/1357.2;
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 4.5f ) {
                                        return 436.0/2592.3;
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.413614630699f ) {
                                            return 374.0/1383.7;
                                        } else {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.173749998212f ) {
                                                return 378.0/1735.7;
                                            } else {
                                                return 324.0/1786.5;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 17.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.34582144022f ) {
                                        return 179.0/1688.9;
                                    } else {
                                        return 210.0/2618.8;
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 26.5f ) {
                                        return 249.0/1920.8;
                                    } else {
                                        return 251.0/1733.6;
                                    }
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 2326.5f ) {
                                return 333.0/1778.4;
                            } else {
                                if ( cl->size() <= 14.5f ) {
                                    return 342.0/1426.4;
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 57.5f ) {
                                        return 632.0/1920.8;
                                    } else {
                                        return 487.0/1072.3;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.num_overlap_literals <= 28.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 21812.0f ) {
                                if ( cl->size() <= 9.5f ) {
                                    return 444.0/2211.8;
                                } else {
                                    return 455.0/1467.1;
                                }
                            } else {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 4.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 23.5f ) {
                                        return 583.0/1902.5;
                                    } else {
                                        return 566.0/1428.4;
                                    }
                                } else {
                                    return 658.0/1310.4;
                                }
                            }
                        } else {
                            if ( cl->stats.glue <= 9.5f ) {
                                return 601.0/1277.8;
                            } else {
                                return 706.0/1043.8;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.324999272823f ) {
                        if ( cl->size() <= 8.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 12055.5f ) {
                                if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                    if ( cl->stats.size_rel <= 0.257157683372f ) {
                                        if ( rdb0_last_touched_diff <= 2272.5f ) {
                                            return 163.0/2368.5;
                                        } else {
                                            if ( cl->stats.dump_number <= 11.5f ) {
                                                return 189.0/1790.6;
                                            } else {
                                                return 229.0/2492.6;
                                            }
                                        }
                                    } else {
                                        return 216.0/3414.4;
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 0.240180969238f ) {
                                        return 344.0/2415.3;
                                    } else {
                                        return 171.0/1770.3;
                                    }
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.0920152813196f ) {
                                    return 534.0/2331.9;
                                } else {
                                    return 326.0/2325.8;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 14720.0f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 34.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.104298576713f ) {
                                        return 286.0/1737.7;
                                    } else {
                                        return 293.0/2712.4;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0615440160036f ) {
                                        return 319.0/1808.9;
                                    } else {
                                        return 350.0/2633.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.150827974081f ) {
                                    return 371.0/1265.6;
                                } else {
                                    return 410.0/1806.9;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                            if ( cl->size() <= 39.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 15799.5f ) {
                                    if ( cl->stats.glue <= 7.5f ) {
                                        return 323.0/1713.3;
                                    } else {
                                        return 382.0/1833.3;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.692986130714f ) {
                                        return 330.0/1495.6;
                                    } else {
                                        return 526.0/1570.9;
                                    }
                                }
                            } else {
                                return 485.0/1163.9;
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 77.5f ) {
                                return 319.0/3186.5;
                            } else {
                                return 311.0/1731.6;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                    if ( cl->stats.used_for_uip_creation <= 9.5f ) {
                        if ( cl->stats.glue_rel_queue <= 0.777691364288f ) {
                            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 12.5f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.0631099194288f ) {
                                            return 160.0/1723.5;
                                        } else {
                                            if ( cl->stats.glue <= 6.5f ) {
                                                if ( cl->stats.rdb1_used_for_uip_creation <= 7.5f ) {
                                                    return 141.0/2567.9;
                                                } else {
                                                    return 127.0/3288.2;
                                                }
                                            } else {
                                                return 191.0/2405.1;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.473679453135f ) {
                                            if ( cl->stats.dump_number <= 11.5f ) {
                                                return 121.0/2897.5;
                                            } else {
                                                return 87.0/3241.4;
                                            }
                                        } else {
                                            return 155.0/3235.3;
                                        }
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 2605.5f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.0592729300261f ) {
                                            return 197.0/1788.6;
                                        } else {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                                if ( cl->stats.num_antecedents_rel <= 0.187956213951f ) {
                                                    return 180.0/2964.7;
                                                } else {
                                                    return 163.0/1941.2;
                                                }
                                            } else {
                                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.104070216417f ) {
                                                    if ( rdb0_last_touched_diff <= 1064.0f ) {
                                                        return 72.0/2329.8;
                                                    } else {
                                                        return 93.0/1924.9;
                                                    }
                                                } else {
                                                    return 127.0/1825.2;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.size_rel <= 0.417525291443f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 8991.5f ) {
                                                if ( cl->stats.num_total_lits_antecedents <= 16.5f ) {
                                                    return 200.0/2338.0;
                                                } else {
                                                    return 249.0/2150.8;
                                                }
                                            } else {
                                                return 213.0/1741.8;
                                            }
                                        } else {
                                            return 309.0/2038.9;
                                        }
                                    }
                                }
                            } else {
                                return 213.0/1961.5;
                            }
                        } else {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 7.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 56.5f ) {
                                    return 262.0/2875.1;
                                } else {
                                    return 391.0/2411.2;
                                }
                            } else {
                                return 183.0/2907.7;
                            }
                        }
                    } else {
                        if ( cl->size() <= 30.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 7173.5f ) {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                                    if ( rdb0_last_touched_diff <= 2668.5f ) {
                                        if ( cl->stats.used_for_uip_creation <= 14.5f ) {
                                            return 112.0/2136.5;
                                        } else {
                                            return 131.0/3811.1;
                                        }
                                    } else {
                                        return 186.0/1998.2;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 11.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.26150316f ) {
                                            return 148.0/3345.2;
                                        } else {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0312480386347f ) {
                                                if ( cl->stats.rdb1_last_touched_diff <= 1270.0f ) {
                                                    return 67.0/1990.0;
                                                } else {
                                                    return 85.0/1935.1;
                                                }
                                            } else {
                                                if ( cl->size() <= 6.5f ) {
                                                    if ( cl->stats.used_for_uip_creation <= 17.5f ) {
                                                        return 56.0/2101.9;
                                                    } else {
                                                        return 31.0/2354.2;
                                                    }
                                                } else {
                                                    if ( cl->stats.glue_rel_queue <= 0.576394081116f ) {
                                                        return 127.0/3286.2;
                                                    } else {
                                                        return 84.0/3005.4;
                                                    }
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.184146225452f ) {
                                            if ( cl->stats.size_rel <= 0.0629631876945f ) {
                                                if ( cl->stats.glue_rel_long <= 0.12470972538f ) {
                                                    return 60.0/2077.5;
                                                } else {
                                                    return 37.0/2055.1;
                                                }
                                            } else {
                                                return 147.0/3131.5;
                                            }
                                        } else {
                                            if ( cl->stats.num_overlap_literals <= 4.5f ) {
                                                if ( cl->stats.used_for_uip_creation <= 27.5f ) {
                                                    if ( rdb0_last_touched_diff <= 1968.0f ) {
                                                        return 64.0/4049.2;
                                                    } else {
                                                        return 68.0/2055.1;
                                                    }
                                                } else {
                                                    if ( cl->stats.num_overlap_literals_rel <= 0.0314845517278f ) {
                                                        if ( cl->stats.glue <= 4.5f ) {
                                                            return 58.0/3082.7;
                                                        } else {
                                                            return 31.0/2348.1;
                                                        }
                                                    } else {
                                                        return 8.0/2940.3;
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.used_for_uip_creation <= 19.5f ) {
                                                    if ( cl->size() <= 13.5f ) {
                                                        if ( cl->stats.num_overlap_literals_rel <= 0.0558663457632f ) {
                                                            return 88.0/2293.2;
                                                        } else {
                                                            return 85.0/3805.0;
                                                        }
                                                    } else {
                                                        return 94.0/1906.6;
                                                    }
                                                } else {
                                                    if ( cl->stats.size_rel <= 0.141767561436f ) {
                                                        if ( cl->stats.num_overlap_literals <= 8.5f ) {
                                                            if ( cl->stats.glue_rel_long <= 0.327031821012f ) {
                                                                return 72.0/1920.8;
                                                            } else {
                                                                return 59.0/2030.7;
                                                            }
                                                        } else {
                                                            if ( cl->stats.dump_number <= 9.5f ) {
                                                                return 45.0/2146.7;
                                                            } else {
                                                                return 41.0/2215.9;
                                                            }
                                                        }
                                                    } else {
                                                        if ( rdb0_last_touched_diff <= 2540.5f ) {
                                                            if ( cl->stats.antec_num_total_lits_rel <= 0.150401771069f ) {
                                                                if ( cl->stats.glue_rel_long <= 0.52801412344f ) {
                                                                    if ( cl->size() <= 9.5f ) {
                                                                        return 34.0/3121.4;
                                                                    } else {
                                                                        return 42.0/2250.5;
                                                                    }
                                                                } else {
                                                                    return 53.0/2002.2;
                                                                }
                                                            } else {
                                                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.247448980808f ) {
                                                                    return 19.0/3680.9;
                                                                } else {
                                                                    return 36.0/2136.5;
                                                                }
                                                            }
                                                        } else {
                                                            return 133.0/2362.4;
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.29859662056f ) {
                                    return 148.0/1786.5;
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 17.5f ) {
                                        return 198.0/2842.6;
                                    } else {
                                        return 92.0/2789.7;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 21.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 96.5f ) {
                                    return 118.0/3628.0;
                                } else {
                                    return 198.0/3664.6;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0369272045791f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0137029299513f ) {
                                        return 44.0/2185.4;
                                    } else {
                                        return 54.0/2128.4;
                                    }
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.653333306313f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 641.0f ) {
                                            return 21.0/3837.6;
                                        } else {
                                            return 34.0/2401.0;
                                        }
                                    } else {
                                        return 58.0/3514.1;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.glue_rel_queue <= 0.631725072861f ) {
                        if ( cl->stats.dump_number <= 11.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 21051.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.155238643289f ) {
                                    if ( cl->size() <= 8.5f ) {
                                        return 172.0/3343.1;
                                    } else {
                                        return 169.0/2171.1;
                                    }
                                } else {
                                    return 150.0/3609.7;
                                }
                            } else {
                                return 290.0/2801.9;
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 11168.5f ) {
                                return 143.0/2240.3;
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 12.5f ) {
                                    if ( cl->size() <= 7.5f ) {
                                        return 204.0/1739.7;
                                    } else {
                                        return 277.0/1823.2;
                                    }
                                } else {
                                    return 140.0/2262.7;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.237187504768f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                                return 222.0/3084.7;
                            } else {
                                return 272.0/2747.0;
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 17423.5f ) {
                                return 275.0/2451.9;
                            } else {
                                return 449.0/2565.9;
                            }
                        }
                    }
                }
            }
        }
    } else {
        if ( cl->stats.antecedents_glue_long_reds_var <= 0.250887513161f ) {
            if ( rdb0_last_touched_diff <= 55940.5f ) {
                if ( cl->stats.glue_rel_long <= 0.717918634415f ) {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                        if ( cl->stats.glue_rel_long <= 0.255364179611f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 17089.5f ) {
                                return 408.0/1485.4;
                            } else {
                                return 681.0/1650.2;
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 34166.5f ) {
                                if ( cl->stats.num_overlap_literals <= 8.5f ) {
                                    if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                        if ( cl->stats.glue_rel_long <= 0.439171254635f ) {
                                            return 409.0/1231.0;
                                        } else {
                                            return 388.0/1347.0;
                                        }
                                    } else {
                                        return 835.0/2034.8;
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 3.5f ) {
                                        return 879.0/1558.6;
                                    } else {
                                        return 810.0/1776.4;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 12.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 18.5f ) {
                                        return 983.0/1823.2;
                                    } else {
                                        return 1031.0/1434.5;
                                    }
                                } else {
                                    return 1028.0/1237.1;
                                }
                            }
                        }
                    } else {
                        if ( cl->size() <= 8.5f ) {
                            return 297.0/2193.5;
                        } else {
                            return 288.0/1619.7;
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_last_touched_diff <= 7655.5f ) {
                        return 833.0/2205.7;
                    } else {
                        if ( cl->stats.size_rel <= 0.97926056385f ) {
                            if ( cl->stats.glue_rel_long <= 1.05894947052f ) {
                                if ( cl->stats.dump_number <= 5.5f ) {
                                    if ( rdb0_last_touched_diff <= 33490.5f ) {
                                        return 684.0/1049.9;
                                    } else {
                                        return 713.0/651.1;
                                    }
                                } else {
                                    return 762.0/1432.5;
                                }
                            } else {
                                return 947.0/974.7;
                            }
                        } else {
                            return 1158.0/834.3;
                        }
                    }
                }
            } else {
                if ( rdb0_last_touched_diff <= 156709.5f ) {
                    if ( cl->stats.num_overlap_literals <= 16.5f ) {
                        if ( cl->stats.size_rel <= 0.486981838942f ) {
                            if ( cl->stats.size_rel <= 0.176112234592f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.0370202101767f ) {
                                    return 730.0/722.3;
                                } else {
                                    if ( cl->stats.glue <= 3.5f ) {
                                        return 626.0/1080.5;
                                    } else {
                                        if ( rdb0_last_touched_diff <= 93683.5f ) {
                                            return 595.0/1035.7;
                                        } else {
                                            return 688.0/858.7;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 69022.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 19.5f ) {
                                        return 707.0/999.1;
                                    } else {
                                        return 662.0/730.5;
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.231334462762f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0813666731119f ) {
                                            return 756.0/618.6;
                                        } else {
                                            if ( cl->size() <= 8.5f ) {
                                                return 880.0/1220.9;
                                            } else {
                                                return 769.0/838.3;
                                            }
                                        }
                                    } else {
                                        return 753.0/567.7;
                                    }
                                }
                            }
                        } else {
                            if ( cl->size() <= 11.5f ) {
                                return 847.0/872.9;
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.20241317153f ) {
                                    if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                        return 804.0/539.2;
                                    } else {
                                        return 796.0/488.3;
                                    }
                                } else {
                                    return 1558.0/803.7;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.dump_number <= 15.5f ) {
                            if ( rdb0_last_touched_diff <= 116670.0f ) {
                                if ( rdb0_last_touched_diff <= 91170.5f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.278264194727f ) {
                                        return 872.0/620.6;
                                    } else {
                                        return 871.0/394.7;
                                    }
                                } else {
                                    return 864.0/431.4;
                                }
                            } else {
                                return 1141.0/388.6;
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.631600856781f ) {
                                return 723.0/647.1;
                            } else {
                                return 757.0/604.3;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_overlap_literals <= 5.5f ) {
                        if ( cl->stats.glue <= 5.5f ) {
                            if ( rdb0_last_touched_diff <= 276895.0f ) {
                                return 957.0/952.3;
                            } else {
                                return 892.0/602.3;
                            }
                        } else {
                            if ( cl->stats.glue <= 8.5f ) {
                                return 1021.0/641.0;
                            } else {
                                return 1224.0/541.3;
                            }
                        }
                    } else {
                        if ( cl->stats.glue <= 5.5f ) {
                            if ( cl->stats.size_rel <= 0.176323622465f ) {
                                return 1299.0/1113.0;
                            } else {
                                if ( cl->stats.dump_number <= 42.5f ) {
                                    return 1096.0/468.0;
                                } else {
                                    return 831.0/435.4;
                                }
                            }
                        } else {
                            if ( cl->stats.size_rel <= 0.597797214985f ) {
                                if ( cl->stats.num_overlap_literals <= 18.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 304186.5f ) {
                                        if ( rdb0_last_touched_diff <= 214002.0f ) {
                                            return 851.0/486.3;
                                        } else {
                                            return 819.0/577.9;
                                        }
                                    } else {
                                        return 1309.0/439.5;
                                    }
                                } else {
                                    return 1721.0/638.9;
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 259118.5f ) {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 7.5f ) {
                                        return 979.0/421.2;
                                    } else {
                                        return 1400.0/295.0;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.816187500954f ) {
                                        return 1140.0/236.0;
                                    } else {
                                        return 1543.0/221.8;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.glue <= 9.5f ) {
                if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                    if ( cl->size() <= 11.5f ) {
                        if ( cl->stats.glue <= 6.5f ) {
                            if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                return 820.0/1741.8;
                            } else {
                                return 745.0/1049.9;
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 49359.0f ) {
                                return 880.0/1259.5;
                            } else {
                                return 871.0/592.1;
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 62545.0f ) {
                            if ( cl->stats.size_rel <= 0.869300961494f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.197118714452f ) {
                                    return 741.0/653.2;
                                } else {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                                        return 552.0/952.3;
                                    } else {
                                        return 749.0/838.3;
                                    }
                                }
                            } else {
                                return 1139.0/785.4;
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.756391346455f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 117973.0f ) {
                                    return 1003.0/586.0;
                                } else {
                                    return 881.0/392.7;
                                }
                            } else {
                                return 1256.0/384.6;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.size_rel <= 0.663159251213f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 45.5f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 5.5f ) {
                                return 1182.0/1070.3;
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 223244.5f ) {
                                    return 1386.0/1001.1;
                                } else {
                                    return 985.0/455.8;
                                }
                            }
                        } else {
                            if ( cl->stats.size_rel <= 0.307334542274f ) {
                                if ( cl->stats.glue_rel_long <= 0.650596261024f ) {
                                    return 795.0/584.0;
                                } else {
                                    return 847.0/417.1;
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.393073379993f ) {
                                    return 1248.0/339.8;
                                } else {
                                    if ( cl->stats.dump_number <= 13.5f ) {
                                        return 827.0/508.7;
                                    } else {
                                        return 1139.0/429.3;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 38.5f ) {
                            return 1142.0/480.2;
                        } else {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 8.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 1.06730091572f ) {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 5.5f ) {
                                        return 1596.0/588.1;
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 2.72218751907f ) {
                                            return 999.0/307.3;
                                        } else {
                                            return 1104.0/278.8;
                                        }
                                    }
                                } else {
                                    return 927.0/215.7;
                                }
                            } else {
                                return 1535.0/175.0;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.rdb1_last_touched_diff <= 9089.0f ) {
                    if ( cl->stats.glue_rel_long <= 0.910173356533f ) {
                        return 649.0/801.7;
                    } else {
                        if ( cl->stats.num_antecedents_rel <= 0.363517522812f ) {
                            return 820.0/455.8;
                        } else {
                            return 1525.0/423.2;
                        }
                    }
                } else {
                    if ( cl->stats.num_total_lits_antecedents <= 156.5f ) {
                        if ( cl->stats.size_rel <= 1.0363060236f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                                if ( cl->stats.glue_rel_queue <= 0.958260774612f ) {
                                    return 1755.0/1198.5;
                                } else {
                                    return 1712.0/643.0;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 20.5f ) {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 6.5f ) {
                                        if ( rdb0_last_touched_diff <= 129229.5f ) {
                                            return 874.0/535.1;
                                        } else {
                                            return 957.0/268.6;
                                        }
                                    } else {
                                        return 1566.0/470.0;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.910556435585f ) {
                                        if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                            return 1587.0/661.3;
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 0.781698346138f ) {
                                                return 1292.0/480.2;
                                            } else {
                                                return 1065.0/252.3;
                                            }
                                        }
                                    } else {
                                        if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 11.1889057159f ) {
                                                if ( rdb0_last_touched_diff <= 146526.5f ) {
                                                    return 1251.0/366.3;
                                                } else {
                                                    return 1062.0/225.9;
                                                }
                                            } else {
                                                if ( rdb0_last_touched_diff <= 83510.0f ) {
                                                    return 906.0/203.5;
                                                } else {
                                                    return 1661.0/268.6;
                                                }
                                            }
                                        } else {
                                            return 1480.0/160.7;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 44847.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 26666.5f ) {
                                    return 997.0/227.9;
                                } else {
                                    return 978.0/337.8;
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 10.6887493134f ) {
                                        if ( cl->stats.size_rel <= 1.44658207893f ) {
                                            if ( cl->stats.num_antecedents_rel <= 0.392810791731f ) {
                                                return 883.0/301.1;
                                            } else {
                                                return 1806.0/409.0;
                                            }
                                        } else {
                                            return 1352.0/193.3;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 1.12498283386f ) {
                                            return 1674.0/343.9;
                                        } else {
                                            if ( cl->stats.dump_number <= 17.5f ) {
                                                return 1194.0/87.5;
                                            } else {
                                                return 1037.0/142.4;
                                            }
                                        }
                                    }
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 9.5f ) {
                                        return 1310.0/162.8;
                                    } else {
                                        return 1426.0/81.4;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_queue <= 0.930189847946f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                                return 1185.0/451.7;
                            } else {
                                if ( rdb0_last_touched_diff <= 58040.0f ) {
                                    return 866.0/293.0;
                                } else {
                                    if ( cl->size() <= 60.5f ) {
                                        if ( cl->stats.glue_rel_long <= 0.815082967281f ) {
                                            return 872.0/331.7;
                                        } else {
                                            return 1169.0/262.5;
                                        }
                                    } else {
                                        return 1835.0/276.7;
                                    }
                                }
                            }
                        } else {
                            if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                if ( cl->stats.glue <= 16.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 1.63007831573f ) {
                                        if ( cl->stats.rdb1_act_ranking_top_10 <= 4.5f ) {
                                            return 1920.0/234.0;
                                        } else {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.969441890717f ) {
                                                return 970.0/187.2;
                                            } else {
                                                return 1132.0/150.6;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.dump_number <= 18.5f ) {
                                            if ( cl->stats.glue <= 12.5f ) {
                                                return 930.0/201.4;
                                            } else {
                                                return 1516.0/201.4;
                                            }
                                        } else {
                                            return 966.0/272.7;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 2.01519536972f ) {
                                        if ( rdb0_last_touched_diff <= 63712.0f ) {
                                            if ( cl->stats.num_overlap_literals <= 581.5f ) {
                                                if ( cl->stats.antecedents_glue_long_reds_var <= 23.9517612457f ) {
                                                    return 925.0/205.5;
                                                } else {
                                                    if ( cl->stats.glue_rel_queue <= 1.12410092354f ) {
                                                        return 1044.0/189.2;
                                                    } else {
                                                        return 1695.0/240.1;
                                                    }
                                                }
                                            } else {
                                                return 1466.0/128.2;
                                            }
                                        } else {
                                            if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                                if ( cl->stats.dump_number <= 17.5f ) {
                                                    if ( cl->stats.num_total_lits_antecedents <= 288.5f ) {
                                                        return 965.0/81.4;
                                                    } else {
                                                        if ( cl->size() <= 97.5f ) {
                                                            return 1611.0/91.6;
                                                        } else {
                                                            return 1064.0/22.4;
                                                        }
                                                    }
                                                } else {
                                                    if ( rdb0_last_touched_diff <= 230639.0f ) {
                                                        return 935.0/152.6;
                                                    } else {
                                                        return 1462.0/113.9;
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.rdb1_last_touched_diff <= 112328.0f ) {
                                                    return 1296.0/223.8;
                                                } else {
                                                    return 1575.0/142.4;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.930368542671f ) {
                                            return 1052.0/28.5;
                                        } else {
                                            if ( cl->size() <= 114.5f ) {
                                                return 978.0/109.9;
                                            } else {
                                                return 1596.0/73.3;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.size_rel <= 1.2342325449f ) {
                                    if ( cl->stats.num_overlap_literals <= 181.5f ) {
                                        return 1080.0/144.5;
                                    } else {
                                        return 1798.0/181.1;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 1.18476724625f ) {
                                        return 1088.0/77.3;
                                    } else {
                                        if ( cl->stats.glue <= 27.5f ) {
                                            return 1102.0/26.5;
                                        } else {
                                            return 1228.0/16.3;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

static double estimator_should_keep_short_conf2_cluster0_6(
    const CMSat::Clause* cl
    , const uint64_t sumConflicts
    , const uint32_t rdb0_last_touched_diff
    , const uint32_t rdb0_act_ranking
    , const uint32_t rdb0_act_ranking_top_10
) {

    double rdb_rel_used_for_uip_creation = 0;
    if (cl->stats.used_for_uip_creation > cl->stats.rdb1_used_for_uip_creation) {
        rdb_rel_used_for_uip_creation = 1.0;
    } else {
        rdb_rel_used_for_uip_creation = 0.0;
    }

    double rdb0_avg_confl;
    if (cl->stats.sum_delta_confl_uip1_used == 0) {
        rdb0_avg_confl = 0;
    } else {
        rdb0_avg_confl = ((double)cl->stats.sum_uip1_used)/((double)cl->stats.sum_delta_confl_uip1_used);
    }

    double rdb0_used_per_confl;
    if (sumConflicts-cl->stats.introduced_at_conflict == 0) {
        rdb0_used_per_confl = 0;
    } else {
        rdb0_used_per_confl = ((double)cl->stats.sum_uip1_used)/((double)sumConflicts-(double)cl->stats.introduced_at_conflict);
    }
    if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
        if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
            if ( cl->stats.num_total_lits_antecedents <= 73.5f ) {
                if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                    if ( cl->stats.glue <= 7.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 31030.0f ) {
                            if ( cl->size() <= 9.5f ) {
                                if ( cl->stats.glue <= 5.5f ) {
                                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                        if ( cl->size() <= 5.5f ) {
                                            if ( cl->size() <= 4.5f ) {
                                                return 482.0/2274.9;
                                            } else {
                                                return 413.0/1638.0;
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 0.3867380023f ) {
                                                return 551.0/1487.4;
                                            } else {
                                                return 572.0/2061.2;
                                            }
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 29718.0f ) {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.119316458702f ) {
                                                if ( cl->stats.glue_rel_queue <= 0.402279615402f ) {
                                                    return 738.0/2529.2;
                                                } else {
                                                    return 555.0/1497.6;
                                                }
                                            } else {
                                                return 662.0/2462.1;
                                            }
                                        } else {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.0871301442385f ) {
                                                return 523.0/1281.9;
                                            } else {
                                                return 440.0/1369.4;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 22.5f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 16.5f ) {
                                            return 394.0/1438.6;
                                        } else {
                                            return 826.0/2354.2;
                                        }
                                    } else {
                                        return 643.0/1601.4;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 17377.0f ) {
                                    if ( cl->stats.dump_number <= 2.5f ) {
                                        return 813.0/1367.4;
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0718222633004f ) {
                                            return 620.0/1794.7;
                                        } else {
                                            return 963.0/2118.2;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 25209.0f ) {
                                        return 988.0/1766.2;
                                    } else {
                                        return 615.0/921.8;
                                    }
                                }
                            }
                        } else {
                            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.112549260259f ) {
                                    return 691.0/1861.8;
                                } else {
                                    return 731.0/1552.5;
                                }
                            } else {
                                if ( cl->size() <= 9.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 61730.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0662150084972f ) {
                                            return 643.0/1047.9;
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 0.48755300045f ) {
                                                return 619.0/1436.6;
                                            } else {
                                                return 539.0/940.1;
                                            }
                                        }
                                    } else {
                                        return 1078.0/1463.0;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 51176.5f ) {
                                        return 820.0/1119.1;
                                    } else {
                                        return 1016.0/909.5;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 46164.0f ) {
                            if ( cl->stats.size_rel <= 0.850471019745f ) {
                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.81733751297f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 28.5f ) {
                                            return 510.0/1296.2;
                                        } else {
                                            if ( cl->stats.size_rel <= 0.491300165653f ) {
                                                return 540.0/999.1;
                                            } else {
                                                return 471.0/1133.4;
                                            }
                                        }
                                    } else {
                                        return 853.0/1450.8;
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 2.5f ) {
                                        return 933.0/942.1;
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 24280.5f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.122173383832f ) {
                                                return 811.0/1800.8;
                                            } else {
                                                return 607.0/1056.1;
                                            }
                                        } else {
                                            return 832.0/1031.6;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.425204455853f ) {
                                    return 1279.0/1511.8;
                                } else {
                                    return 837.0/655.2;
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.945029914379f ) {
                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                    return 1101.0/1463.0;
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 31.5f ) {
                                        return 733.0/789.5;
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 19.5f ) {
                                            return 818.0/569.7;
                                        } else {
                                            return 731.0/622.6;
                                        }
                                    }
                                }
                            } else {
                                return 1061.0/571.8;
                            }
                        }
                    }
                } else {
                    if ( rdb0_last_touched_diff <= 2230.5f ) {
                        if ( rdb0_last_touched_diff <= 715.5f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.0493827164173f ) {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                                    if ( cl->stats.size_rel <= 0.196535751224f ) {
                                        return 181.0/1988.0;
                                    } else {
                                        return 191.0/3292.3;
                                    }
                                } else {
                                    return 134.0/2661.5;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 13.5f ) {
                                    return 152.0/2071.4;
                                } else {
                                    return 166.0/1804.9;
                                }
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 39.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 19258.0f ) {
                                    if ( rdb0_last_touched_diff <= 1261.5f ) {
                                        return 249.0/3044.0;
                                    } else {
                                        if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                            return 280.0/1680.7;
                                        } else {
                                            return 201.0/2484.5;
                                        }
                                    }
                                } else {
                                    return 369.0/2480.4;
                                }
                            } else {
                                return 368.0/2034.8;
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 16646.5f ) {
                            if ( cl->stats.num_antecedents_rel <= 0.0390513166785f ) {
                                return 352.0/1397.9;
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 22.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.515181303024f ) {
                                            return 515.0/2722.5;
                                        } else {
                                            return 241.0/1617.7;
                                        }
                                    } else {
                                        if ( cl->stats.size_rel <= 0.513234972954f ) {
                                            return 543.0/2163.0;
                                        } else {
                                            return 467.0/1355.2;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0664087682962f ) {
                                        return 264.0/1835.4;
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.245905771852f ) {
                                            if ( cl->stats.num_overlap_literals <= 6.5f ) {
                                                return 173.0/2128.4;
                                            } else {
                                                return 273.0/2097.9;
                                            }
                                        } else {
                                            return 292.0/2144.7;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->size() <= 12.5f ) {
                                if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                                    return 636.0/2283.0;
                                } else {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                        return 302.0/2108.0;
                                    } else {
                                        return 364.0/1939.1;
                                    }
                                }
                            } else {
                                return 711.0/2399.0;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.glue <= 10.5f ) {
                    if ( cl->stats.glue <= 5.5f ) {
                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                            return 573.0/2643.2;
                        } else {
                            return 673.0/1357.2;
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 9667.5f ) {
                            if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                return 593.0/1652.2;
                            } else {
                                return 305.0/2124.3;
                            }
                        } else {
                            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.515704393387f ) {
                                    return 1054.0/1239.2;
                                } else {
                                    if ( cl->stats.size_rel <= 0.587690114975f ) {
                                        return 522.0/1017.4;
                                    } else {
                                        return 664.0/864.8;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 42029.5f ) {
                                    if ( cl->size() <= 18.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.871284246445f ) {
                                            return 559.0/942.1;
                                        } else {
                                            return 686.0/667.4;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.758434414864f ) {
                                            return 744.0/651.1;
                                        } else {
                                            return 886.0/563.6;
                                        }
                                    }
                                } else {
                                    return 1419.0/811.9;
                                }
                            }
                        }
                    }
                } else {
                    if ( rdb0_last_touched_diff <= 9784.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 19338.0f ) {
                            if ( cl->size() <= 61.5f ) {
                                return 373.0/2014.4;
                            } else {
                                return 363.0/1444.7;
                            }
                        } else {
                            return 384.0/1430.5;
                        }
                    } else {
                        if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                            if ( cl->stats.dump_number <= 16.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.83842921257f ) {
                                    if ( cl->stats.glue_rel_queue <= 1.05054998398f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.895024895668f ) {
                                            return 1312.0/1190.3;
                                        } else {
                                            return 1086.0/647.1;
                                        }
                                    } else {
                                        return 1633.0/474.1;
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 49340.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 1.00030326843f ) {
                                            return 762.0/569.7;
                                        } else {
                                            if ( cl->stats.glue <= 22.5f ) {
                                                return 1385.0/439.5;
                                            } else {
                                                return 999.0/116.0;
                                            }
                                        }
                                    } else {
                                        return 1078.0/146.5;
                                    }
                                }
                            } else {
                                return 895.0/1383.7;
                            }
                        } else {
                            if ( cl->stats.dump_number <= 14.5f ) {
                                if ( cl->stats.num_overlap_literals <= 181.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.9443385005f ) {
                                        return 1263.0/622.6;
                                    } else {
                                        if ( cl->stats.size_rel <= 1.07127332687f ) {
                                            return 1375.0/392.7;
                                        } else {
                                            return 1405.0/238.1;
                                        }
                                    }
                                } else {
                                    if ( cl->size() <= 84.5f ) {
                                        if ( cl->stats.glue <= 19.5f ) {
                                            return 1025.0/262.5;
                                        } else {
                                            return 978.0/97.7;
                                        }
                                    } else {
                                        return 1575.0/116.0;
                                    }
                                }
                            } else {
                                return 1077.0/844.4;
                            }
                        }
                    }
                }
            }
        } else {
            if ( rdb0_last_touched_diff <= 9858.5f ) {
                if ( cl->stats.rdb1_used_for_uip_creation <= 8.5f ) {
                    if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                        if ( cl->size() <= 11.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 6942.0f ) {
                                if ( cl->stats.size_rel <= 0.254617452621f ) {
                                    if ( cl->size() <= 6.5f ) {
                                        if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                                            return 336.0/2325.8;
                                        } else {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 5.5f ) {
                                                return 272.0/2877.2;
                                            } else {
                                                return 175.0/2663.5;
                                            }
                                        }
                                    } else {
                                        return 366.0/2431.6;
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 3937.0f ) {
                                        if ( cl->size() <= 7.5f ) {
                                            return 133.0/2163.0;
                                        } else {
                                            return 147.0/2203.7;
                                        }
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 18.5f ) {
                                            return 163.0/1994.1;
                                        } else {
                                            return 225.0/1737.7;
                                        }
                                    }
                                }
                            } else {
                                return 324.0/2252.5;
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 59.5f ) {
                                if ( rdb0_last_touched_diff <= 4143.5f ) {
                                    if ( cl->stats.size_rel <= 0.510703802109f ) {
                                        return 231.0/1579.0;
                                    } else {
                                        return 163.0/1904.6;
                                    }
                                } else {
                                    return 457.0/2783.6;
                                }
                            } else {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 4.5f ) {
                                    return 679.0/2468.2;
                                } else {
                                    return 372.0/2588.2;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.used_for_uip_creation <= 10.5f ) {
                            if ( cl->size() <= 32.5f ) {
                                if ( rdb0_last_touched_diff <= 2825.0f ) {
                                    if ( cl->stats.num_overlap_literals <= 14.5f ) {
                                        if ( cl->stats.used_for_uip_creation <= 5.5f ) {
                                            return 174.0/2478.4;
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 1299.0f ) {
                                                return 117.0/1945.3;
                                            } else {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.0844019651413f ) {
                                                    return 100.0/1992.1;
                                                } else {
                                                    return 73.0/2108.0;
                                                }
                                            }
                                        }
                                    } else {
                                        return 197.0/2533.3;
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 17.5f ) {
                                        return 218.0/3025.7;
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 14.5f ) {
                                            return 239.0/2024.6;
                                        } else {
                                            return 258.0/1751.9;
                                        }
                                    }
                                }
                            } else {
                                return 439.0/2637.1;
                            }
                        } else {
                            if ( cl->stats.num_antecedents_rel <= 0.0469102747738f ) {
                                return 218.0/3518.1;
                            } else {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.248456791043f ) {
                                    if ( cl->stats.used_for_uip_creation <= 16.5f ) {
                                        if ( rdb0_last_touched_diff <= 1502.5f ) {
                                            return 103.0/2936.2;
                                        } else {
                                            return 140.0/2517.0;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.297872543335f ) {
                                            return 65.0/1951.4;
                                        } else {
                                            if ( rdb0_last_touched_diff <= 1413.0f ) {
                                                return 64.0/3674.8;
                                            } else {
                                                return 68.0/1994.1;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 77.5f ) {
                                        return 137.0/3479.5;
                                    } else {
                                        return 111.0/1920.8;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( rdb0_last_touched_diff <= 2864.5f ) {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 20.5f ) {
                            if ( cl->stats.num_antecedents_rel <= 0.0281198304147f ) {
                                return 195.0/3398.1;
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0191085524857f ) {
                                    return 162.0/3837.6;
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 69.5f ) {
                                        if ( cl->stats.used_for_uip_creation <= 9.5f ) {
                                            if ( rdb0_last_touched_diff <= 428.5f ) {
                                                return 50.0/2181.3;
                                            } else {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.104188844562f ) {
                                                    return 122.0/2246.4;
                                                } else {
                                                    return 147.0/3300.4;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.145929217339f ) {
                                                if ( cl->stats.num_overlap_literals <= 5.5f ) {
                                                    return 45.0/4079.7;
                                                } else {
                                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0831609815359f ) {
                                                        return 64.0/2222.0;
                                                    } else {
                                                        return 41.0/2531.3;
                                                    }
                                                }
                                            } else {
                                                if ( rdb0_last_touched_diff <= 745.0f ) {
                                                    return 30.0/2983.0;
                                                } else {
                                                    return 89.0/2419.4;
                                                }
                                            }
                                        }
                                    } else {
                                        return 106.0/2285.1;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 14.5f ) {
                                if ( cl->stats.glue_rel_queue <= 0.302571952343f ) {
                                    return 91.0/2311.5;
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0538161657751f ) {
                                        return 99.0/3037.9;
                                    } else {
                                        return 77.0/3414.4;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.0890466123819f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0126219298691f ) {
                                        if ( cl->stats.used_for_uip_creation <= 51.5f ) {
                                            return 40.0/2079.5;
                                        } else {
                                            return 28.0/2673.7;
                                        }
                                    } else {
                                        if ( cl->stats.size_rel <= 0.11894082278f ) {
                                            if ( cl->size() <= 5.5f ) {
                                                return 108.0/2909.7;
                                            } else {
                                                return 48.0/2248.4;
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 251.5f ) {
                                                return 39.0/2932.1;
                                            } else {
                                                if ( cl->stats.num_antecedents_rel <= 0.0519287437201f ) {
                                                    return 52.0/1994.1;
                                                } else {
                                                    return 46.0/2266.7;
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.236053198576f ) {
                                        return 61.0/3508.0;
                                    } else {
                                        if ( cl->stats.dump_number <= 28.5f ) {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.146863117814f ) {
                                                if ( cl->stats.num_overlap_literals <= 7.5f ) {
                                                    return 26.0/3939.3;
                                                } else {
                                                    return 69.0/2887.4;
                                                }
                                            } else {
                                                if ( cl->stats.used_for_uip_creation <= 28.5f ) {
                                                    return 43.0/2230.1;
                                                } else {
                                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.407075971365f ) {
                                                        if ( cl->stats.num_overlap_literals_rel <= 0.1677044034f ) {
                                                            return 4.0/2930.1;
                                                        } else {
                                                            return 12.0/2057.2;
                                                        }
                                                    } else {
                                                        return 27.0/2256.6;
                                                    }
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0987119600177f ) {
                                                return 12.0/2893.5;
                                            } else {
                                                return 14.0/2071.4;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 4.50761795044f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 26.5f ) {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 13.5f ) {
                                    if ( cl->stats.size_rel <= 0.252716004848f ) {
                                        return 199.0/2683.9;
                                    } else {
                                        return 113.0/1865.9;
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 5773.5f ) {
                                        if ( cl->size() <= 4.5f ) {
                                            return 59.0/2069.4;
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 0.398247778416f ) {
                                                return 115.0/1835.4;
                                            } else {
                                                return 87.0/2346.1;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.070124194026f ) {
                                            return 174.0/1808.9;
                                        } else {
                                            return 105.0/1918.8;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 38.5f ) {
                                    if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                        return 330.0/2738.8;
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 1375.5f ) {
                                            return 216.0/2690.0;
                                        } else {
                                            return 136.0/2217.9;
                                        }
                                    }
                                } else {
                                    return 94.0/2470.2;
                                }
                            }
                        } else {
                            return 215.0/1951.4;
                        }
                    }
                }
            } else {
                if ( cl->size() <= 10.5f ) {
                    if ( cl->stats.rdb1_last_touched_diff <= 3700.5f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 21.5f ) {
                            if ( cl->stats.num_overlap_literals <= 6.5f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.0693016797304f ) {
                                    return 462.0/2889.4;
                                } else {
                                    return 344.0/3113.2;
                                }
                            } else {
                                return 327.0/1920.8;
                            }
                        } else {
                            if ( cl->size() <= 7.5f ) {
                                return 258.0/1640.0;
                            } else {
                                return 427.0/1943.2;
                            }
                        }
                    } else {
                        if ( cl->stats.num_overlap_literals_rel <= 0.0231175757945f ) {
                            return 414.0/1524.1;
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.576645970345f ) {
                                if ( cl->stats.dump_number <= 14.5f ) {
                                    return 513.0/3001.3;
                                } else {
                                    return 519.0/2358.3;
                                }
                            } else {
                                return 569.0/2191.5;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_overlap_literals_rel <= 0.571075439453f ) {
                        if ( cl->stats.glue_rel_queue <= 0.983376324177f ) {
                            if ( rdb0_last_touched_diff <= 13145.5f ) {
                                if ( cl->size() <= 36.5f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                                        return 580.0/1654.3;
                                    } else {
                                        if ( cl->stats.size_rel <= 0.413106709719f ) {
                                            return 243.0/1607.5;
                                        } else {
                                            return 376.0/1918.8;
                                        }
                                    }
                                } else {
                                    return 475.0/1361.3;
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.128399074078f ) {
                                        return 713.0/2283.0;
                                    } else {
                                        return 572.0/1540.3;
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 0.542917609215f ) {
                                        return 631.0/1581.0;
                                    } else {
                                        return 614.0/1277.8;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals <= 26.5f ) {
                                return 461.0/1178.1;
                            } else {
                                return 661.0/960.4;
                            }
                        }
                    } else {
                        if ( cl->stats.dump_number <= 1.5f ) {
                            return 911.0/826.1;
                        } else {
                            return 575.0/1475.2;
                        }
                    }
                }
            }
        }
    } else {
        if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
            if ( cl->size() <= 11.5f ) {
                if ( cl->size() <= 7.5f ) {
                    if ( cl->stats.rdb1_act_ranking_top_10 <= 5.5f ) {
                        if ( rdb0_last_touched_diff <= 37550.0f ) {
                            if ( cl->stats.glue_rel_long <= 0.514519810677f ) {
                                return 740.0/2474.3;
                            } else {
                                return 482.0/1186.3;
                            }
                        } else {
                            if ( cl->stats.num_antecedents_rel <= 0.143375530839f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.105288892984f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 108880.0f ) {
                                        return 1073.0/1937.1;
                                    } else {
                                        return 705.0/813.9;
                                    }
                                } else {
                                    return 545.0/1045.9;
                                }
                            } else {
                                if ( cl->stats.dump_number <= 31.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 60402.5f ) {
                                        return 858.0/1397.9;
                                    } else {
                                        return 1080.0/1314.5;
                                    }
                                } else {
                                    return 838.0/854.6;
                                }
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 82199.0f ) {
                            return 808.0/1432.5;
                        } else {
                            if ( cl->stats.dump_number <= 66.5f ) {
                                if ( rdb0_act_ranking_top_10 <= 7.5f ) {
                                    return 1382.0/1292.1;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 211397.0f ) {
                                        return 1359.0/1121.2;
                                    } else {
                                        return 859.0/451.7;
                                    }
                                }
                            } else {
                                return 724.0/868.9;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                        if ( rdb0_last_touched_diff <= 52971.0f ) {
                            if ( cl->stats.glue_rel_queue <= 0.648403286934f ) {
                                return 565.0/1210.7;
                            } else {
                                return 830.0/1123.2;
                            }
                        } else {
                            if ( cl->stats.glue <= 6.5f ) {
                                return 816.0/828.2;
                            } else {
                                return 988.0/665.4;
                            }
                        }
                    } else {
                        if ( cl->stats.antec_num_total_lits_rel <= 0.988036334515f ) {
                            if ( cl->stats.num_overlap_literals <= 7.5f ) {
                                if ( rdb0_last_touched_diff <= 125312.5f ) {
                                    return 1152.0/1688.9;
                                } else {
                                    if ( rdb0_last_touched_diff <= 262964.0f ) {
                                        return 1102.0/915.7;
                                    } else {
                                        return 851.0/417.1;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 146607.0f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.685923337936f ) {
                                        if ( rdb0_last_touched_diff <= 66901.5f ) {
                                            return 631.0/783.4;
                                        } else {
                                            return 1304.0/1133.4;
                                        }
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.486631929874f ) {
                                            return 1012.0/931.9;
                                        } else {
                                            return 1199.0/608.4;
                                        }
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 285277.5f ) {
                                        if ( cl->stats.dump_number <= 22.5f ) {
                                            return 895.0/295.0;
                                        } else {
                                            return 1190.0/773.2;
                                        }
                                    } else {
                                        return 1591.0/571.8;
                                    }
                                }
                            }
                        } else {
                            return 1323.0/305.2;
                        }
                    }
                }
            } else {
                if ( cl->stats.antec_num_total_lits_rel <= 0.475917875767f ) {
                    if ( cl->stats.size_rel <= 0.663177251816f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 166.5f ) {
                            if ( cl->stats.dump_number <= 21.5f ) {
                                if ( rdb0_last_touched_diff <= 56140.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.72548019886f ) {
                                        if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 23997.5f ) {
                                                return 682.0/1286.0;
                                            } else {
                                                return 758.0/905.5;
                                            }
                                        } else {
                                            return 642.0/1206.6;
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 25.5f ) {
                                            return 986.0/919.7;
                                        } else {
                                            return 1108.0/575.8;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.6145439744f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.425449639559f ) {
                                            return 1008.0/1131.3;
                                        } else {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.228298604488f ) {
                                                return 764.0/681.7;
                                            } else {
                                                return 950.0/508.7;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.131143003702f ) {
                                            return 1490.0/325.6;
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 49.5f ) {
                                                return 1379.0/728.5;
                                            } else {
                                                if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                                    return 1167.0/317.4;
                                                } else {
                                                    return 976.0/380.5;
                                                }
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 14.5f ) {
                                    if ( cl->stats.dump_number <= 71.5f ) {
                                        if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                            return 1064.0/620.6;
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 30.5f ) {
                                                return 1000.0/358.1;
                                            } else {
                                                return 860.0/327.6;
                                            }
                                        }
                                    } else {
                                        return 786.0/527.0;
                                    }
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 4.23111104965f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 182114.0f ) {
                                            return 806.0/590.1;
                                        } else {
                                            if ( cl->stats.size_rel <= 0.484913885593f ) {
                                                return 1374.0/500.6;
                                            } else {
                                                return 951.0/211.6;
                                            }
                                        }
                                    } else {
                                        return 1327.0/335.7;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.777984499931f ) {
                                return 826.0/407.0;
                            } else {
                                if ( cl->stats.num_overlap_literals <= 159.5f ) {
                                    return 959.0/154.6;
                                } else {
                                    return 948.0/227.9;
                                }
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 70928.0f ) {
                            if ( cl->size() <= 122.5f ) {
                                if ( rdb0_last_touched_diff <= 40465.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.224858760834f ) {
                                        return 885.0/724.4;
                                    } else {
                                        return 770.0/693.9;
                                    }
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.110607206821f ) {
                                        return 768.0/663.3;
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 51.5f ) {
                                            return 852.0/421.2;
                                        } else {
                                            return 1102.0/313.4;
                                        }
                                    }
                                }
                            } else {
                                return 1056.0/254.3;
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 117.5f ) {
                                if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                    if ( rdb0_last_touched_diff <= 330481.0f ) {
                                        if ( cl->size() <= 44.5f ) {
                                            if ( cl->stats.glue_rel_long <= 0.869828760624f ) {
                                                if ( cl->stats.dump_number <= 28.5f ) {
                                                    if ( cl->stats.antecedents_glue_long_reds_var <= 1.77777779102f ) {
                                                        return 1688.0/514.8;
                                                    } else {
                                                        return 1215.0/518.9;
                                                    }
                                                } else {
                                                    return 805.0/451.7;
                                                }
                                            } else {
                                                if ( cl->stats.size_rel <= 0.847411751747f ) {
                                                    return 1034.0/343.9;
                                                } else {
                                                    if ( cl->stats.num_overlap_literals <= 17.5f ) {
                                                        return 938.0/270.6;
                                                    } else {
                                                        return 963.0/187.2;
                                                    }
                                                }
                                            }
                                        } else {
                                            return 1088.0/478.2;
                                        }
                                    } else {
                                        if ( cl->stats.dump_number <= 54.5f ) {
                                            return 1041.0/197.4;
                                        } else {
                                            return 929.0/240.1;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.967484533787f ) {
                                        if ( rdb0_last_touched_diff <= 245901.5f ) {
                                            return 1220.0/374.4;
                                        } else {
                                            return 912.0/197.4;
                                        }
                                    } else {
                                        return 1108.0/177.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 205.5f ) {
                                    return 1860.0/396.8;
                                } else {
                                    return 1638.0/209.6;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_total_lits_antecedents <= 235.5f ) {
                        if ( rdb0_last_touched_diff <= 73594.0f ) {
                            if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 8.8671875f ) {
                                    if ( cl->stats.dump_number <= 5.5f ) {
                                        return 1136.0/697.9;
                                    } else {
                                        return 780.0/761.0;
                                    }
                                } else {
                                    return 1594.0/445.6;
                                }
                            } else {
                                if ( cl->stats.size_rel <= 0.857956051826f ) {
                                    return 1085.0/527.0;
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.510157823563f ) {
                                        return 1054.0/409.0;
                                    } else {
                                        if ( cl->stats.dump_number <= 4.5f ) {
                                            return 934.0/229.9;
                                        } else {
                                            return 1113.0/229.9;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 9.94834136963f ) {
                                if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.883779644966f ) {
                                        if ( cl->stats.rdb1_act_ranking_top_10 <= 4.5f ) {
                                            return 999.0/518.9;
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 0.765963852406f ) {
                                                return 1557.0/429.3;
                                            } else {
                                                return 1023.0/402.9;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.548537731171f ) {
                                            return 1579.0/441.5;
                                        } else {
                                            if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                                return 1150.0/179.1;
                                            } else {
                                                if ( rdb0_last_touched_diff <= 218840.5f ) {
                                                    if ( cl->stats.glue_rel_long <= 1.0869641304f ) {
                                                        return 956.0/291.0;
                                                    } else {
                                                        return 970.0/203.5;
                                                    }
                                                } else {
                                                    return 1448.0/244.2;
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.920585334301f ) {
                                        return 1659.0/339.8;
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 2.03234577179f ) {
                                            return 1145.0/183.1;
                                        } else {
                                            return 1284.0/109.9;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.862506568432f ) {
                                    return 971.0/288.9;
                                } else {
                                    if ( cl->stats.glue_rel_long <= 1.1928331852f ) {
                                        if ( cl->stats.rdb1_act_ranking_top_10 <= 7.5f ) {
                                            if ( cl->stats.glue_rel_long <= 1.07557034492f ) {
                                                return 1739.0/242.1;
                                            } else {
                                                return 965.0/203.5;
                                            }
                                        } else {
                                            return 1436.0/156.7;
                                        }
                                    } else {
                                        if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                            if ( cl->stats.num_total_lits_antecedents <= 114.5f ) {
                                                return 1086.0/142.4;
                                            } else {
                                                if ( cl->size() <= 33.5f ) {
                                                    return 989.0/59.0;
                                                } else {
                                                    return 976.0/95.6;
                                                }
                                            }
                                        } else {
                                            return 1312.0/59.0;
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue <= 8.5f ) {
                            return 1191.0/529.0;
                        } else {
                            if ( cl->stats.glue_rel_queue <= 1.05357933044f ) {
                                if ( cl->stats.glue <= 16.5f ) {
                                    if ( cl->stats.size_rel <= 0.885748445988f ) {
                                        return 1760.0/486.3;
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 1.52925896645f ) {
                                            return 973.0/154.6;
                                        } else {
                                            return 924.0/254.3;
                                        }
                                    }
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                        if ( cl->stats.glue <= 28.5f ) {
                                            return 1357.0/205.5;
                                        } else {
                                            return 1388.0/154.6;
                                        }
                                    } else {
                                        return 1789.0/315.4;
                                    }
                                }
                            } else {
                                if ( cl->stats.size_rel <= 1.96422922611f ) {
                                    if ( rdb0_last_touched_diff <= 107111.0f ) {
                                        if ( cl->size() <= 57.5f ) {
                                            if ( cl->stats.num_overlap_literals <= 455.5f ) {
                                                return 1825.0/211.6;
                                            } else {
                                                return 1058.0/63.1;
                                            }
                                        } else {
                                            if ( cl->stats.dump_number <= 5.5f ) {
                                                return 1808.0/185.2;
                                            } else {
                                                return 1528.0/315.4;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.dump_number <= 17.5f ) {
                                            if ( cl->stats.glue_rel_queue <= 1.18077039719f ) {
                                                return 1007.0/89.5;
                                            } else {
                                                return 1644.0/75.3;
                                            }
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 2.40414381027f ) {
                                                if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                                    return 1210.0/75.3;
                                                } else {
                                                    return 1412.0/148.5;
                                                }
                                            } else {
                                                return 1027.0/173.0;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 607.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 1.26973676682f ) {
                                            return 1067.0/103.8;
                                        } else {
                                            return 1785.0/111.9;
                                        }
                                    } else {
                                        return 1566.0/44.8;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.rdb1_last_touched_diff <= 16194.5f ) {
                if ( rdb0_last_touched_diff <= 2050.0f ) {
                    return 288.0/3426.6;
                } else {
                    return 541.0/2728.6;
                }
            } else {
                if ( rdb0_last_touched_diff <= 2514.0f ) {
                    if ( rdb0_last_touched_diff <= 859.5f ) {
                        if ( cl->stats.glue <= 5.5f ) {
                            return 179.0/2185.4;
                        } else {
                            if ( cl->size() <= 20.5f ) {
                                return 193.0/1998.2;
                            } else {
                                return 209.0/1758.1;
                            }
                        }
                    } else {
                        if ( cl->stats.antec_num_total_lits_rel <= 0.242872267962f ) {
                            if ( cl->stats.num_antecedents_rel <= 0.131367325783f ) {
                                return 313.0/1550.5;
                            } else {
                                return 223.0/1611.5;
                            }
                        } else {
                            return 415.0/1485.4;
                        }
                    }
                } else {
                    if ( cl->stats.glue_rel_long <= 0.685287237167f ) {
                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                            if ( cl->stats.num_overlap_literals <= 11.5f ) {
                                return 656.0/2588.2;
                            } else {
                                return 529.0/1369.4;
                            }
                        } else {
                            return 557.0/1218.8;
                        }
                    } else {
                        if ( cl->size() <= 14.5f ) {
                            return 529.0/1117.1;
                        } else {
                            return 1002.0/1674.6;
                        }
                    }
                }
            }
        }
    }
}

static double estimator_should_keep_short_conf2_cluster0_7(
    const CMSat::Clause* cl
    , const uint64_t sumConflicts
    , const uint32_t rdb0_last_touched_diff
    , const uint32_t rdb0_act_ranking
    , const uint32_t rdb0_act_ranking_top_10
) {

    double rdb_rel_used_for_uip_creation = 0;
    if (cl->stats.used_for_uip_creation > cl->stats.rdb1_used_for_uip_creation) {
        rdb_rel_used_for_uip_creation = 1.0;
    } else {
        rdb_rel_used_for_uip_creation = 0.0;
    }

    double rdb0_avg_confl;
    if (cl->stats.sum_delta_confl_uip1_used == 0) {
        rdb0_avg_confl = 0;
    } else {
        rdb0_avg_confl = ((double)cl->stats.sum_uip1_used)/((double)cl->stats.sum_delta_confl_uip1_used);
    }

    double rdb0_used_per_confl;
    if (sumConflicts-cl->stats.introduced_at_conflict == 0) {
        rdb0_used_per_confl = 0;
    } else {
        rdb0_used_per_confl = ((double)cl->stats.sum_uip1_used)/((double)sumConflicts-(double)cl->stats.introduced_at_conflict);
    }
    if ( rdb0_last_touched_diff <= 21092.5f ) {
        if ( cl->stats.glue_rel_queue <= 0.835382580757f ) {
            if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                    if ( cl->stats.glue <= 6.5f ) {
                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                            if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                                if ( cl->size() <= 12.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.266015797853f ) {
                                        return 411.0/1357.2;
                                    } else {
                                        if ( rdb0_last_touched_diff <= 12218.0f ) {
                                            return 381.0/1924.9;
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 0.405899077654f ) {
                                                return 415.0/1918.8;
                                            } else {
                                                if ( cl->stats.glue_rel_queue <= 0.538696408272f ) {
                                                    return 397.0/1591.2;
                                                } else {
                                                    return 447.0/1564.7;
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    return 479.0/1180.2;
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.569710373878f ) {
                                    if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                                        if ( rdb0_last_touched_diff <= 4890.0f ) {
                                            return 350.0/2342.0;
                                        } else {
                                            return 347.0/1646.1;
                                        }
                                    } else {
                                        return 233.0/2175.2;
                                    }
                                } else {
                                    return 312.0/1534.2;
                                }
                            }
                        } else {
                            if ( cl->size() <= 10.5f ) {
                                if ( rdb0_last_touched_diff <= 10551.0f ) {
                                    return 365.0/1477.3;
                                } else {
                                    if ( cl->stats.size_rel <= 0.237807124853f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.0874325782061f ) {
                                            return 456.0/1137.4;
                                        } else {
                                            return 697.0/2234.2;
                                        }
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.182432413101f ) {
                                            return 373.0/1387.7;
                                        } else {
                                            return 441.0/1513.9;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 5113.5f ) {
                                    return 533.0/976.7;
                                } else {
                                    return 587.0/988.9;
                                }
                            }
                        }
                    } else {
                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 7730.0f ) {
                                if ( rdb0_last_touched_diff <= 9605.5f ) {
                                    return 551.0/2720.5;
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.2469099015f ) {
                                        if ( cl->stats.num_overlap_literals <= 12.5f ) {
                                            return 591.0/1699.0;
                                        } else {
                                            return 552.0/1222.9;
                                        }
                                    } else {
                                        return 434.0/1275.8;
                                    }
                                }
                            } else {
                                return 805.0/1870.0;
                            }
                        } else {
                            if ( cl->size() <= 37.5f ) {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 40.5f ) {
                                        return 595.0/1558.6;
                                    } else {
                                        return 620.0/927.9;
                                    }
                                } else {
                                    return 853.0/1326.7;
                                }
                            } else {
                                return 953.0/879.0;
                            }
                        }
                    }
                } else {
                    if ( rdb0_last_touched_diff <= 3706.5f ) {
                        if ( cl->stats.num_overlap_literals <= 14.5f ) {
                            if ( cl->stats.num_antecedents_rel <= 0.0668714493513f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 22902.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 4369.5f ) {
                                        return 194.0/2319.7;
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.364885538816f ) {
                                            return 202.0/2081.6;
                                        } else {
                                            return 319.0/2156.9;
                                        }
                                    }
                                } else {
                                    return 350.0/1835.4;
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 12317.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.114056557417f ) {
                                        if ( cl->stats.glue_rel_long <= 0.477159142494f ) {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.0664240047336f ) {
                                                return 185.0/2747.0;
                                            } else {
                                                if ( cl->stats.num_overlap_literals_rel <= 0.0462238825858f ) {
                                                    return 106.0/1947.3;
                                                } else {
                                                    return 93.0/1908.6;
                                                }
                                            }
                                        } else {
                                            return 146.0/1782.5;
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 653.5f ) {
                                            return 103.0/2771.4;
                                        } else {
                                            if ( cl->stats.size_rel <= 0.471295446157f ) {
                                                return 156.0/3105.1;
                                            } else {
                                                return 140.0/1737.7;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0942023098469f ) {
                                            return 454.0/2274.9;
                                        } else {
                                            if ( rdb0_last_touched_diff <= 1323.5f ) {
                                                return 269.0/2608.6;
                                            } else {
                                                return 527.0/2474.3;
                                            }
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 238.0f ) {
                                            return 59.0/2215.9;
                                        } else {
                                            if ( cl->stats.used_for_uip_creation <= 11.5f ) {
                                                return 285.0/3162.1;
                                            } else {
                                                return 128.0/2675.7;
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                                    if ( cl->stats.num_overlap_literals <= 48.5f ) {
                                        if ( rdb0_last_touched_diff <= 893.5f ) {
                                            return 161.0/1902.5;
                                        } else {
                                            return 402.0/2380.7;
                                        }
                                    } else {
                                        return 306.0/1908.6;
                                    }
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 5.5f ) {
                                        return 279.0/1790.6;
                                    } else {
                                        return 242.0/3597.5;
                                    }
                                }
                            } else {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.186549991369f ) {
                                    return 217.0/1751.9;
                                } else {
                                    return 528.0/2356.3;
                                }
                            }
                        }
                    } else {
                        if ( cl->size() <= 13.5f ) {
                            if ( cl->stats.num_antecedents_rel <= 0.0361169688404f ) {
                                return 382.0/1330.7;
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 19.5f ) {
                                        if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                            return 372.0/2085.7;
                                        } else {
                                            return 614.0/2228.1;
                                        }
                                    } else {
                                        if ( cl->stats.dump_number <= 14.5f ) {
                                            return 492.0/2014.4;
                                        } else {
                                            return 510.0/1432.5;
                                        }
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 6024.5f ) {
                                        if ( rdb0_last_touched_diff <= 4661.5f ) {
                                            return 323.0/2559.8;
                                        } else {
                                            return 339.0/3149.8;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.493624091148f ) {
                                            return 374.0/2572.0;
                                        } else {
                                            return 317.0/1815.0;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                    return 947.0/2138.6;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 11038.5f ) {
                                        return 264.0/1587.1;
                                    } else {
                                        return 359.0/1790.6;
                                    }
                                }
                            } else {
                                return 616.0/1454.9;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.antecedents_glue_long_reds_var <= 0.0256602317095f ) {
                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                        if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                            if ( rdb0_last_touched_diff <= 11623.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.097305752337f ) {
                                    return 310.0/2226.1;
                                } else {
                                    if ( rdb0_last_touched_diff <= 5819.5f ) {
                                        return 144.0/1790.6;
                                    } else {
                                        return 279.0/2323.7;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 7.5f ) {
                                    return 494.0/3031.8;
                                } else {
                                    return 335.0/1680.7;
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 2956.5f ) {
                                if ( cl->stats.used_for_uip_creation <= 8.5f ) {
                                    if ( cl->size() <= 7.5f ) {
                                        if ( cl->stats.dump_number <= 5.5f ) {
                                            return 167.0/2431.6;
                                        } else {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.0490178763866f ) {
                                                return 109.0/1969.7;
                                            } else {
                                                if ( cl->stats.num_overlap_literals_rel <= 0.0649321824312f ) {
                                                    return 113.0/2983.0;
                                                } else {
                                                    return 53.0/2024.6;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0980761796236f ) {
                                            return 155.0/1900.5;
                                        } else {
                                            return 143.0/3060.3;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0260848850012f ) {
                                        if ( rdb0_last_touched_diff <= 788.5f ) {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.00883525051177f ) {
                                                return 37.0/3449.0;
                                            } else {
                                                if ( cl->stats.used_for_uip_creation <= 19.5f ) {
                                                    return 99.0/2561.8;
                                                } else {
                                                    if ( cl->stats.glue_rel_queue <= 0.202338159084f ) {
                                                        return 82.0/2734.7;
                                                    } else {
                                                        if ( cl->stats.num_antecedents_rel <= 0.0295578539371f ) {
                                                            return 49.0/1996.1;
                                                        } else {
                                                            if ( cl->stats.dump_number <= 9.5f ) {
                                                                return 28.0/2061.2;
                                                            } else {
                                                                return 24.0/2409.2;
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.used_for_uip_creation <= 17.5f ) {
                                                return 146.0/2531.3;
                                            } else {
                                                if ( rdb0_last_touched_diff <= 1419.5f ) {
                                                    return 66.0/2598.4;
                                                } else {
                                                    return 85.0/2565.9;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 12.5f ) {
                                            if ( rdb0_last_touched_diff <= 1063.5f ) {
                                                if ( cl->stats.glue_rel_queue <= 0.429623663425f ) {
                                                    return 53.0/3396.1;
                                                } else {
                                                    return 75.0/2826.3;
                                                }
                                            } else {
                                                return 108.0/3237.3;
                                            }
                                        } else {
                                            if ( cl->stats.size_rel <= 0.315168499947f ) {
                                                if ( cl->stats.rdb1_last_touched_diff <= 2361.0f ) {
                                                    if ( cl->stats.dump_number <= 7.5f ) {
                                                        if ( cl->stats.glue_rel_long <= 0.354103207588f ) {
                                                            return 55.0/2295.2;
                                                        } else {
                                                            return 24.0/2488.5;
                                                        }
                                                    } else {
                                                        if ( cl->stats.num_overlap_literals <= 6.5f ) {
                                                            return 23.0/3693.1;
                                                        } else {
                                                            if ( cl->stats.num_overlap_literals <= 14.5f ) {
                                                                return 31.0/2101.9;
                                                            } else {
                                                                return 18.0/2081.6;
                                                            }
                                                        }
                                                    }
                                                } else {
                                                    return 58.0/2187.4;
                                                }
                                            } else {
                                                if ( cl->stats.size_rel <= 0.522405266762f ) {
                                                    if ( cl->stats.num_overlap_literals <= 7.5f ) {
                                                        return 10.0/2510.9;
                                                    } else {
                                                        return 35.0/2392.9;
                                                    }
                                                } else {
                                                    return 7.0/2842.6;
                                                }
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 10.5f ) {
                                    if ( cl->size() <= 8.5f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.170023441315f ) {
                                            if ( rdb0_last_touched_diff <= 6238.5f ) {
                                                return 236.0/3552.7;
                                            } else {
                                                return 173.0/1729.6;
                                            }
                                        } else {
                                            return 97.0/2268.8;
                                        }
                                    } else {
                                        return 331.0/2928.1;
                                    }
                                } else {
                                    if ( cl->stats.glue <= 5.5f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.0906674191356f ) {
                                            return 115.0/1916.8;
                                        } else {
                                            return 116.0/2720.5;
                                        }
                                    } else {
                                        return 98.0/2643.2;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.num_overlap_literals <= 5.5f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 12.5f ) {
                                if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                                    return 231.0/1823.2;
                                } else {
                                    return 100.0/1965.6;
                                }
                            } else {
                                return 254.0/2081.6;
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 2013.5f ) {
                                if ( cl->stats.glue_rel_queue <= 0.439913421869f ) {
                                    return 167.0/1939.1;
                                } else {
                                    return 190.0/1776.4;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0711199194193f ) {
                                    return 288.0/1524.1;
                                } else {
                                    return 258.0/1906.6;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                        if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                            if ( cl->size() <= 11.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.202051013708f ) {
                                    return 325.0/2484.5;
                                } else {
                                    return 276.0/1532.2;
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 84.5f ) {
                                    if ( cl->stats.dump_number <= 4.5f ) {
                                        return 365.0/1316.5;
                                    } else {
                                        return 343.0/1495.6;
                                    }
                                } else {
                                    return 437.0/1450.8;
                                }
                            }
                        } else {
                            if ( cl->size() <= 15.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.870479941368f ) {
                                    return 240.0/2543.5;
                                } else {
                                    return 254.0/2077.5;
                                }
                            } else {
                                return 423.0/2523.1;
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 12.5f ) {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 5.5f ) {
                                return 282.0/3672.8;
                            } else {
                                if ( rdb0_last_touched_diff <= 2573.0f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.194193512201f ) {
                                        return 141.0/3434.7;
                                    } else {
                                        return 71.0/2154.8;
                                    }
                                } else {
                                    return 219.0/2797.8;
                                }
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 25.5f ) {
                                if ( rdb0_last_touched_diff <= 1262.0f ) {
                                    return 47.0/3078.6;
                                } else {
                                    return 77.0/2437.7;
                                }
                            } else {
                                if ( cl->size() <= 27.5f ) {
                                    if ( rdb0_last_touched_diff <= 2711.5f ) {
                                        if ( cl->stats.size_rel <= 0.265723645687f ) {
                                            return 51.0/2913.8;
                                        } else {
                                            return 81.0/2877.2;
                                        }
                                    } else {
                                        return 165.0/1886.2;
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 1025.5f ) {
                                        return 40.0/2781.5;
                                    } else {
                                        return 85.0/2016.5;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if ( rdb0_last_touched_diff <= 9626.5f ) {
                if ( cl->stats.num_overlap_literals <= 101.5f ) {
                    if ( cl->stats.num_total_lits_antecedents <= 21.5f ) {
                        if ( cl->stats.glue_rel_queue <= 0.998820781708f ) {
                            return 257.0/2421.4;
                        } else {
                            return 224.0/3145.8;
                        }
                    } else {
                        if ( cl->stats.num_overlap_literals_rel <= 0.31663697958f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 17203.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.409081637859f ) {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 6.5f ) {
                                            if ( rdb0_last_touched_diff <= 2782.0f ) {
                                                return 187.0/1981.9;
                                            } else {
                                                return 289.0/1583.1;
                                            }
                                        } else {
                                            if ( cl->stats.used_for_uip_creation <= 12.5f ) {
                                                return 114.0/1981.9;
                                            } else {
                                                if ( cl->stats.num_antecedents_rel <= 0.181293904781f ) {
                                                    return 60.0/1916.8;
                                                } else {
                                                    return 24.0/2158.9;
                                                }
                                            }
                                        }
                                    } else {
                                        return 262.0/2036.8;
                                    }
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                        return 529.0/2093.8;
                                    } else {
                                        if ( cl->stats.size_rel <= 0.573241531849f ) {
                                            return 135.0/2079.5;
                                        } else {
                                            return 197.0/2073.4;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 16.5f ) {
                                    return 415.0/1666.5;
                                } else {
                                    return 581.0/1611.5;
                                }
                            }
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.52904510498f ) {
                                    return 448.0/1507.8;
                                } else {
                                    return 358.0/1424.3;
                                }
                            } else {
                                return 178.0/3182.4;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_total_lits_antecedents <= 629.5f ) {
                        if ( cl->stats.used_for_uip_creation <= 6.5f ) {
                            if ( cl->stats.size_rel <= 0.780767440796f ) {
                                return 352.0/1473.2;
                            } else {
                                return 468.0/1257.5;
                            }
                        } else {
                            return 140.0/2327.8;
                        }
                    } else {
                        return 435.0/1448.8;
                    }
                }
            } else {
                if ( cl->stats.glue <= 10.5f ) {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                        if ( cl->stats.size_rel <= 0.726400256157f ) {
                            if ( cl->stats.dump_number <= 2.5f ) {
                                return 982.0/1178.1;
                            } else {
                                return 561.0/1255.5;
                            }
                        } else {
                            return 915.0/661.3;
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 2623.0f ) {
                            return 382.0/1511.8;
                        } else {
                            return 867.0/2112.1;
                        }
                    }
                } else {
                    if ( cl->stats.antec_num_total_lits_rel <= 0.567942738533f ) {
                        if ( cl->stats.glue_rel_queue <= 1.03476381302f ) {
                            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                return 545.0/1182.2;
                            } else {
                                return 748.0/927.9;
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 4.05102062225f ) {
                                return 780.0/1235.1;
                            } else {
                                return 1061.0/594.2;
                            }
                        }
                    } else {
                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                            return 1434.0/1043.8;
                        } else {
                            if ( cl->stats.dump_number <= 1.5f ) {
                                if ( cl->stats.num_overlap_literals <= 176.5f ) {
                                    return 1209.0/297.1;
                                } else {
                                    return 1738.0/152.6;
                                }
                            } else {
                                return 749.0/653.2;
                            }
                        }
                    }
                }
            }
        }
    } else {
        if ( cl->stats.num_overlap_literals <= 37.5f ) {
            if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                if ( cl->stats.glue <= 8.5f ) {
                    if ( cl->stats.rdb1_last_touched_diff <= 44195.5f ) {
                        if ( rdb0_last_touched_diff <= 30741.5f ) {
                            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 23.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.0996220260859f ) {
                                        return 487.0/1534.2;
                                    } else {
                                        return 515.0/2156.9;
                                    }
                                } else {
                                    return 900.0/2285.1;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 17.5f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0313709937036f ) {
                                        return 774.0/1597.3;
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 14792.5f ) {
                                            return 667.0/2217.9;
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.15887516737f ) {
                                                return 478.0/1397.9;
                                            } else {
                                                return 582.0/1324.6;
                                            }
                                        }
                                    }
                                } else {
                                    return 668.0/1115.1;
                                }
                            }
                        } else {
                            if ( cl->size() <= 9.5f ) {
                                if ( cl->stats.dump_number <= 7.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0762018561363f ) {
                                        return 550.0/978.7;
                                    } else {
                                        return 702.0/1768.2;
                                    }
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                        return 606.0/1979.8;
                                    } else {
                                        if ( rdb0_last_touched_diff <= 43888.0f ) {
                                            return 1261.0/3139.7;
                                        } else {
                                            return 802.0/1558.6;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.dump_number <= 5.5f ) {
                                    return 1148.0/1454.9;
                                } else {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                        return 736.0/1530.2;
                                    } else {
                                        return 1133.0/1802.8;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->size() <= 9.5f ) {
                            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                return 463.0/1208.7;
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.279134809971f ) {
                                    return 776.0/862.7;
                                } else {
                                    if ( cl->size() <= 5.5f ) {
                                        return 643.0/1227.0;
                                    } else {
                                        if ( rdb0_last_touched_diff <= 83590.0f ) {
                                            return 1025.0/1489.5;
                                        } else {
                                            return 920.0/938.0;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 38.5f ) {
                                if ( cl->stats.dump_number <= 18.5f ) {
                                    return 1079.0/1296.2;
                                } else {
                                    return 1114.0/962.5;
                                }
                            } else {
                                return 1216.0/801.7;
                            }
                        }
                    }
                } else {
                    if ( rdb0_last_touched_diff <= 44366.5f ) {
                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                            if ( cl->stats.dump_number <= 3.5f ) {
                                return 666.0/862.7;
                            } else {
                                return 993.0/1947.3;
                            }
                        } else {
                            if ( cl->size() <= 15.5f ) {
                                return 752.0/1129.3;
                            } else {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.0546875f ) {
                                    return 694.0/1007.2;
                                } else {
                                    return 1398.0/1092.7;
                                }
                            }
                        }
                    } else {
                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                            if ( cl->stats.size_rel <= 0.67540037632f ) {
                                return 640.0/889.2;
                            } else {
                                return 746.0/586.0;
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 91600.0f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 3.20486116409f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.168917387724f ) {
                                        return 714.0/645.0;
                                    } else {
                                        return 1039.0/771.2;
                                    }
                                } else {
                                    return 1065.0/547.4;
                                }
                            } else {
                                return 1577.0/687.8;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.num_total_lits_antecedents <= 23.5f ) {
                    if ( cl->stats.size_rel <= 0.165195569396f ) {
                        if ( cl->stats.num_overlap_literals_rel <= 0.0324168689549f ) {
                            if ( rdb0_last_touched_diff <= 159490.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.0600086003542f ) {
                                    return 879.0/1406.0;
                                } else {
                                    return 527.0/1039.8;
                                }
                            } else {
                                return 1561.0/1168.0;
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 113018.5f ) {
                                return 600.0/1499.6;
                            } else {
                                return 772.0/791.5;
                            }
                        }
                    } else {
                        if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                            if ( rdb0_last_touched_diff <= 67290.0f ) {
                                if ( cl->stats.glue_rel_long <= 0.590571165085f ) {
                                    return 795.0/1666.5;
                                } else {
                                    return 645.0/919.7;
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 155484.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.597779035568f ) {
                                        return 949.0/1100.8;
                                    } else {
                                        return 687.0/673.5;
                                    }
                                } else {
                                    return 781.0/522.9;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 112223.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.164206206799f ) {
                                    return 823.0/1111.0;
                                } else {
                                    return 692.0/669.4;
                                }
                            } else {
                                if ( cl->stats.size_rel <= 0.498627156019f ) {
                                    if ( cl->stats.glue <= 5.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.429953485727f ) {
                                            return 784.0/502.6;
                                        } else {
                                            return 803.0/700.0;
                                        }
                                    } else {
                                        return 1295.0/771.2;
                                    }
                                } else {
                                    return 1144.0/358.1;
                                }
                            }
                        }
                    }
                } else {
                    if ( rdb0_last_touched_diff <= 80511.5f ) {
                        if ( cl->stats.glue_rel_long <= 0.663552343845f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 24290.5f ) {
                                return 723.0/1259.5;
                            } else {
                                if ( cl->stats.dump_number <= 7.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 40.5f ) {
                                        return 742.0/677.6;
                                    } else {
                                        return 873.0/612.5;
                                    }
                                } else {
                                    return 1133.0/1636.0;
                                }
                            }
                        } else {
                            if ( cl->stats.dump_number <= 8.5f ) {
                                if ( cl->stats.num_overlap_literals <= 19.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 1.05922853947f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 34.5f ) {
                                            return 728.0/641.0;
                                        } else {
                                            return 1115.0/677.6;
                                        }
                                    } else {
                                        return 1104.0/421.2;
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 0.818096399307f ) {
                                        return 1291.0/689.8;
                                    } else {
                                        return 1225.0/347.9;
                                    }
                                }
                            } else {
                                return 1353.0/1483.4;
                            }
                        }
                    } else {
                        if ( cl->size() <= 11.5f ) {
                            if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                if ( cl->stats.glue <= 6.5f ) {
                                    return 1331.0/1186.3;
                                } else {
                                    return 918.0/636.9;
                                }
                            } else {
                                if ( cl->stats.glue <= 5.5f ) {
                                    return 1044.0/687.8;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 187496.5f ) {
                                        return 871.0/480.2;
                                    } else {
                                        return 1050.0/463.9;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.0355029590428f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.492510229349f ) {
                                    if ( cl->size() <= 27.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.126867115498f ) {
                                            if ( rdb0_last_touched_diff <= 198477.0f ) {
                                                return 1006.0/606.4;
                                            } else {
                                                return 1086.0/350.0;
                                            }
                                        } else {
                                            return 1644.0/537.2;
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 152855.5f ) {
                                            return 1119.0/988.9;
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.148710340261f ) {
                                                return 952.0/311.3;
                                            } else {
                                                return 1153.0/533.1;
                                            }
                                        }
                                    }
                                } else {
                                    return 922.0/225.9;
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 48.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.943707704544f ) {
                                        if ( cl->stats.rdb1_act_ranking_top_10 <= 6.5f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 188740.5f ) {
                                                if ( cl->stats.num_overlap_literals <= 13.5f ) {
                                                    return 882.0/374.4;
                                                } else {
                                                    return 808.0/488.3;
                                                }
                                            } else {
                                                return 1035.0/317.4;
                                            }
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.262161761522f ) {
                                                return 1674.0/618.6;
                                            } else {
                                                return 930.0/262.5;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 176085.0f ) {
                                            return 978.0/191.3;
                                        } else {
                                            return 1000.0/122.1;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 151089.0f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.811407446861f ) {
                                            return 1183.0/478.2;
                                        } else {
                                            if ( cl->size() <= 25.5f ) {
                                                return 985.0/221.8;
                                            } else {
                                                return 1190.0/213.7;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.682956457138f ) {
                                            return 1078.0/278.8;
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 0.945746123791f ) {
                                                if ( cl->stats.glue_rel_long <= 0.81594145298f ) {
                                                    return 1029.0/160.7;
                                                } else {
                                                    return 968.0/223.8;
                                                }
                                            } else {
                                                if ( cl->stats.num_overlap_literals_rel <= 0.310281187296f ) {
                                                    return 1343.0/229.9;
                                                } else {
                                                    return 1102.0/93.6;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                if ( cl->stats.size_rel <= 0.540240645409f ) {
                    if ( cl->stats.num_overlap_literals <= 359.5f ) {
                        if ( cl->size() <= 11.5f ) {
                            if ( cl->stats.glue_rel_long <= 0.73592877388f ) {
                                return 616.0/1501.7;
                            } else {
                                return 622.0/824.1;
                            }
                        } else {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.213230386376f ) {
                                return 776.0/923.8;
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 38878.0f ) {
                                    return 918.0/1129.3;
                                } else {
                                    return 829.0/433.4;
                                }
                            }
                        }
                    } else {
                        return 1015.0/468.0;
                    }
                } else {
                    if ( cl->stats.glue_rel_queue <= 1.00151658058f ) {
                        if ( cl->stats.glue_rel_queue <= 0.851487338543f ) {
                            if ( cl->stats.num_overlap_literals_rel <= 0.396823227406f ) {
                                return 1186.0/905.5;
                            } else {
                                if ( rdb0_last_touched_diff <= 47474.5f ) {
                                    return 1184.0/1601.4;
                                } else {
                                    return 1190.0/724.4;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 22575.5f ) {
                                return 732.0/708.1;
                            } else {
                                if ( cl->stats.glue <= 12.5f ) {
                                    return 895.0/531.1;
                                } else {
                                    return 1187.0/439.5;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue <= 13.5f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 6.99382686615f ) {
                                return 886.0/466.0;
                            } else {
                                return 897.0/360.2;
                            }
                        } else {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.967144846916f ) {
                                if ( cl->stats.dump_number <= 5.5f ) {
                                    return 1113.0/240.1;
                                } else {
                                    return 1059.0/443.6;
                                }
                            } else {
                                if ( cl->stats.size_rel <= 1.44430160522f ) {
                                    return 1805.0/341.8;
                                } else {
                                    return 1431.0/142.4;
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.glue_rel_queue <= 0.910768091679f ) {
                    if ( cl->stats.num_antecedents_rel <= 0.440270483494f ) {
                        if ( cl->stats.size_rel <= 0.815500497818f ) {
                            if ( cl->stats.size_rel <= 0.195596754551f ) {
                                return 899.0/683.7;
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.612608671188f ) {
                                    return 1251.0/748.8;
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.217497065663f ) {
                                        return 967.0/217.7;
                                    } else {
                                        return 1337.0/533.1;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 151636.5f ) {
                                return 1523.0/435.4;
                            } else {
                                return 1047.0/124.1;
                            }
                        }
                    } else {
                        if ( cl->size() <= 9.5f ) {
                            return 1347.0/1430.5;
                        } else {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                                return 1535.0/944.1;
                            } else {
                                if ( cl->stats.glue <= 9.5f ) {
                                    if ( cl->stats.dump_number <= 30.5f ) {
                                        if ( cl->stats.glue_rel_long <= 0.593133687973f ) {
                                            return 898.0/588.1;
                                        } else {
                                            if ( cl->stats.size_rel <= 0.831218123436f ) {
                                                return 876.0/390.7;
                                            } else {
                                                return 917.0/272.7;
                                            }
                                        }
                                    } else {
                                        return 1024.0/297.1;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.454669386148f ) {
                                        return 817.0/433.4;
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.87677770853f ) {
                                            return 1633.0/398.8;
                                        } else {
                                            if ( cl->stats.size_rel <= 0.88699400425f ) {
                                                return 1158.0/439.5;
                                            } else {
                                                return 1297.0/311.3;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.glue_rel_queue <= 1.101385355f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 180.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 63492.0f ) {
                                if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                    return 956.0/443.6;
                                } else {
                                    return 1023.0/352.0;
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                    if ( cl->stats.glue <= 9.5f ) {
                                        return 927.0/343.9;
                                    } else {
                                        if ( cl->size() <= 22.5f ) {
                                            return 1544.0/227.9;
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 123.5f ) {
                                                return 947.0/248.2;
                                            } else {
                                                return 1012.0/189.2;
                                            }
                                        }
                                    }
                                } else {
                                    return 1387.0/170.9;
                                }
                            }
                        } else {
                            if ( cl->size() <= 116.5f ) {
                                if ( cl->size() <= 18.5f ) {
                                    return 1597.0/400.9;
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 10.7329521179f ) {
                                        if ( rdb0_last_touched_diff <= 129880.5f ) {
                                            return 916.0/205.5;
                                        } else {
                                            return 932.0/187.2;
                                        }
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 328.5f ) {
                                            return 1611.0/175.0;
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 1.06228423119f ) {
                                                return 1222.0/187.2;
                                            } else {
                                                return 925.0/234.0;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                    return 1677.0/93.6;
                                } else {
                                    return 1262.0/166.9;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue <= 10.5f ) {
                            if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                return 1213.0/516.8;
                            } else {
                                return 972.0/173.0;
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 279.5f ) {
                                if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 1.00295853615f ) {
                                        return 976.0/309.3;
                                    } else {
                                        if ( cl->stats.size_rel <= 1.23220014572f ) {
                                            if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                                if ( cl->stats.num_overlap_literals <= 119.5f ) {
                                                    if ( cl->stats.num_overlap_literals <= 73.5f ) {
                                                        return 1282.0/240.1;
                                                    } else {
                                                        return 1327.0/215.7;
                                                    }
                                                } else {
                                                    return 1377.0/126.2;
                                                }
                                            } else {
                                                return 1416.0/347.9;
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 1.34337067604f ) {
                                                if ( cl->stats.size_rel <= 1.51459574699f ) {
                                                    return 1057.0/113.9;
                                                } else {
                                                    return 1181.0/221.8;
                                                }
                                            } else {
                                                if ( cl->stats.num_overlap_literals_rel <= 1.04194259644f ) {
                                                    return 1058.0/111.9;
                                                } else {
                                                    return 1118.0/44.8;
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 1.03714549541f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 167362.0f ) {
                                            return 959.0/124.1;
                                        } else {
                                            return 990.0/103.8;
                                        }
                                    } else {
                                        return 1416.0/59.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue <= 14.5f ) {
                                    return 1408.0/191.3;
                                } else {
                                    if ( cl->size() <= 373.5f ) {
                                        if ( cl->stats.glue_rel_long <= 1.12691307068f ) {
                                            return 1236.0/193.3;
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 31544.0f ) {
                                                return 1131.0/160.7;
                                            } else {
                                                if ( rdb0_act_ranking_top_10 <= 9.5f ) {
                                                    if ( cl->stats.rdb1_last_touched_diff <= 183427.0f ) {
                                                        if ( cl->stats.num_overlap_literals <= 472.0f ) {
                                                            if ( cl->stats.rdb1_act_ranking_top_10 <= 6.5f ) {
                                                                return 1736.0/109.9;
                                                            } else {
                                                                return 1080.0/156.7;
                                                            }
                                                        } else {
                                                            if ( cl->stats.rdb1_act_ranking_top_10 <= 5.5f ) {
                                                                return 996.0/18.3;
                                                            } else {
                                                                return 1070.0/89.5;
                                                            }
                                                        }
                                                    } else {
                                                        if ( cl->stats.num_antecedents_rel <= 1.67669117451f ) {
                                                            return 1077.0/22.4;
                                                        } else {
                                                            return 994.0/69.2;
                                                        }
                                                    }
                                                } else {
                                                    return 1113.0/24.4;
                                                }
                                            }
                                        }
                                    } else {
                                        return 1301.0/42.7;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

static double estimator_should_keep_short_conf2_cluster0_8(
    const CMSat::Clause* cl
    , const uint64_t sumConflicts
    , const uint32_t rdb0_last_touched_diff
    , const uint32_t rdb0_act_ranking
    , const uint32_t rdb0_act_ranking_top_10
) {

    double rdb_rel_used_for_uip_creation = 0;
    if (cl->stats.used_for_uip_creation > cl->stats.rdb1_used_for_uip_creation) {
        rdb_rel_used_for_uip_creation = 1.0;
    } else {
        rdb_rel_used_for_uip_creation = 0.0;
    }

    double rdb0_avg_confl;
    if (cl->stats.sum_delta_confl_uip1_used == 0) {
        rdb0_avg_confl = 0;
    } else {
        rdb0_avg_confl = ((double)cl->stats.sum_uip1_used)/((double)cl->stats.sum_delta_confl_uip1_used);
    }

    double rdb0_used_per_confl;
    if (sumConflicts-cl->stats.introduced_at_conflict == 0) {
        rdb0_used_per_confl = 0;
    } else {
        rdb0_used_per_confl = ((double)cl->stats.sum_uip1_used)/((double)sumConflicts-(double)cl->stats.introduced_at_conflict);
    }
    if ( cl->stats.glue_rel_queue <= 0.839397192001f ) {
        if ( rdb0_act_ranking_top_10 <= 2.5f ) {
            if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                if ( cl->size() <= 10.5f ) {
                    if ( cl->stats.num_overlap_literals <= 6.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 17403.0f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 4070.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0584183931351f ) {
                                    if ( cl->stats.size_rel <= 0.230871081352f ) {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                            return 436.0/1633.9;
                                        } else {
                                            if ( cl->stats.size_rel <= 0.129516899586f ) {
                                                return 208.0/1743.8;
                                            } else {
                                                return 279.0/1788.6;
                                            }
                                        }
                                    } else {
                                        return 283.0/2392.9;
                                    }
                                } else {
                                    return 295.0/2590.3;
                                }
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                        if ( cl->size() <= 6.5f ) {
                                            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                                return 512.0/2242.3;
                                            } else {
                                                return 554.0/1737.7;
                                            }
                                        } else {
                                            return 800.0/2287.1;
                                        }
                                    } else {
                                        return 480.0/2598.4;
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 0.209416955709f ) {
                                        return 379.0/1841.5;
                                    } else {
                                        return 270.0/2099.9;
                                    }
                                }
                            }
                        } else {
                            if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.0641722679138f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 41520.0f ) {
                                        return 840.0/2160.9;
                                    } else {
                                        return 648.0/1121.2;
                                    }
                                } else {
                                    if ( cl->size() <= 7.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.103533253074f ) {
                                            return 366.0/1328.7;
                                        } else {
                                            return 422.0/1247.3;
                                        }
                                    } else {
                                        return 705.0/1292.1;
                                    }
                                }
                            } else {
                                return 699.0/2360.3;
                            }
                        }
                    } else {
                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                            if ( rdb0_last_touched_diff <= 31493.5f ) {
                                if ( cl->size() <= 7.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.172974139452f ) {
                                        if ( cl->stats.glue_rel_long <= 0.328355193138f ) {
                                            return 556.0/1992.1;
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 5213.5f ) {
                                                return 306.0/2024.6;
                                            } else {
                                                return 480.0/2222.0;
                                            }
                                        }
                                    } else {
                                        return 469.0/2974.9;
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 0.35050034523f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0934458598495f ) {
                                            return 404.0/1646.1;
                                        } else {
                                            return 817.0/2545.5;
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 12215.5f ) {
                                            return 391.0/2197.6;
                                        } else {
                                            return 414.0/1699.0;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.size_rel <= 0.19228091836f ) {
                                    return 479.0/1233.1;
                                } else {
                                    return 502.0/1090.6;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.219464272261f ) {
                                        return 670.0/1713.3;
                                    } else {
                                        return 501.0/1524.1;
                                    }
                                } else {
                                    if ( cl->size() <= 8.5f ) {
                                        if ( cl->stats.glue_rel_long <= 0.384022265673f ) {
                                            return 553.0/1035.7;
                                        } else {
                                            return 811.0/1853.7;
                                        }
                                    } else {
                                        return 735.0/1104.9;
                                    }
                                }
                            } else {
                                return 553.0/2989.1;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 14242.5f ) {
                            if ( cl->stats.num_overlap_literals <= 16.5f ) {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 5.5f ) {
                                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                            if ( cl->stats.num_antecedents_rel <= 0.128119543195f ) {
                                                return 542.0/1577.0;
                                            } else {
                                                return 616.0/1395.9;
                                            }
                                        } else {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.204861104488f ) {
                                                return 353.0/1381.6;
                                            } else {
                                                return 441.0/1340.9;
                                            }
                                        }
                                    } else {
                                        return 886.0/2014.4;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 7.5f ) {
                                        return 240.0/1762.1;
                                    } else {
                                        return 283.0/1666.5;
                                    }
                                }
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                        if ( cl->stats.dump_number <= 2.5f ) {
                                            return 984.0/2126.3;
                                        } else {
                                            if ( cl->stats.size_rel <= 0.696437418461f ) {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.240070939064f ) {
                                                    return 520.0/1461.0;
                                                } else {
                                                    return 394.0/1361.3;
                                                }
                                            } else {
                                                return 561.0/1300.2;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                            return 730.0/956.3;
                                        } else {
                                            return 499.0/1316.5;
                                        }
                                    }
                                } else {
                                    return 654.0/2378.7;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 38121.0f ) {
                                if ( cl->stats.dump_number <= 5.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 20051.0f ) {
                                        return 666.0/1037.7;
                                    } else {
                                        return 1108.0/1279.9;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 30.5f ) {
                                        if ( cl->size() <= 17.5f ) {
                                            return 767.0/2010.4;
                                        } else {
                                            return 567.0/1023.5;
                                        }
                                    } else {
                                        return 759.0/1290.1;
                                    }
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.308143019676f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 61180.5f ) {
                                        return 805.0/1283.9;
                                    } else {
                                        return 700.0/777.3;
                                    }
                                } else {
                                    return 891.0/726.4;
                                }
                            }
                        }
                    } else {
                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                            if ( rdb0_last_touched_diff <= 2747.5f ) {
                                return 451.0/1216.8;
                            } else {
                                return 715.0/1308.4;
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals_rel <= 0.631581783295f ) {
                                if ( cl->stats.glue <= 6.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.193873494864f ) {
                                        return 640.0/769.1;
                                    } else {
                                        return 605.0/877.0;
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 46.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 41081.5f ) {
                                            return 725.0/1273.8;
                                        } else {
                                            return 787.0/754.9;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.586465001106f ) {
                                            return 776.0/651.1;
                                        } else {
                                            return 1299.0/870.9;
                                        }
                                    }
                                }
                            } else {
                                return 752.0/612.5;
                            }
                        }
                    }
                }
            } else {
                if ( rdb0_last_touched_diff <= 2866.5f ) {
                    if ( cl->stats.used_for_uip_creation <= 7.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 10063.5f ) {
                            if ( cl->size() <= 17.5f ) {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.606323242188f ) {
                                        if ( cl->stats.glue_rel_long <= 0.279436349869f ) {
                                            return 165.0/1743.8;
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 15.5f ) {
                                                return 155.0/2698.1;
                                            } else {
                                                return 273.0/2865.0;
                                            }
                                        }
                                    } else {
                                        return 206.0/1804.9;
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 0.168408110738f ) {
                                        if ( cl->size() <= 5.5f ) {
                                            return 234.0/3880.3;
                                        } else {
                                            return 157.0/1833.3;
                                        }
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.195790827274f ) {
                                            if ( cl->size() <= 5.5f ) {
                                                return 65.0/2226.1;
                                            } else {
                                                if ( cl->stats.glue_rel_long <= 0.476226747036f ) {
                                                    return 86.0/2116.2;
                                                } else {
                                                    return 99.0/2057.2;
                                                }
                                            }
                                        } else {
                                            return 152.0/2842.6;
                                        }
                                    }
                                }
                            } else {
                                if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                    return 284.0/3387.9;
                                } else {
                                    return 323.0/2108.0;
                                }
                            }
                        } else {
                            if ( cl->stats.num_antecedents_rel <= 0.0622682720423f ) {
                                return 337.0/1564.7;
                            } else {
                                if ( cl->size() <= 20.5f ) {
                                    if ( rdb0_last_touched_diff <= 1898.0f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.562049031258f ) {
                                            if ( cl->stats.size_rel <= 0.250484019518f ) {
                                                return 235.0/2197.6;
                                            } else {
                                                return 195.0/2698.1;
                                            }
                                        } else {
                                            return 296.0/2340.0;
                                        }
                                    } else {
                                        return 301.0/1835.4;
                                    }
                                } else {
                                    return 281.0/1737.7;
                                }
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 782.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 3066.5f ) {
                                if ( cl->stats.glue_rel_queue <= 0.193532779813f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0291473306715f ) {
                                        return 86.0/3518.1;
                                    } else {
                                        return 105.0/2954.5;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0232614018023f ) {
                                        if ( rdb0_last_touched_diff <= 174.5f ) {
                                            if ( cl->stats.dump_number <= 15.5f ) {
                                                return 23.0/3375.7;
                                            } else {
                                                return 36.0/2042.9;
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 26.5f ) {
                                                return 93.0/2677.8;
                                            } else {
                                                return 47.0/3025.7;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.dump_number <= 28.5f ) {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.127976611257f ) {
                                                if ( cl->stats.used_for_uip_creation <= 15.5f ) {
                                                    return 69.0/2061.2;
                                                } else {
                                                    if ( cl->stats.num_overlap_literals <= 8.5f ) {
                                                        if ( cl->stats.rdb1_last_touched_diff <= 474.5f ) {
                                                            return 23.0/1996.1;
                                                        } else {
                                                            return 16.0/2207.7;
                                                        }
                                                    } else {
                                                        return 60.0/2942.3;
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.rdb1_used_for_uip_creation <= 7.5f ) {
                                                    return 44.0/2016.5;
                                                } else {
                                                    if ( cl->stats.num_overlap_literals <= 19.5f ) {
                                                        return 16.0/4144.9;
                                                    } else {
                                                        if ( cl->stats.used_for_uip_creation <= 40.5f ) {
                                                            return 37.0/2130.4;
                                                        } else {
                                                            return 16.0/2561.8;
                                                        }
                                                    }
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 15.5f ) {
                                                return 12.0/2321.7;
                                            } else {
                                                return 40.0/3772.5;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.0634986460209f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.0660997107625f ) {
                                        return 162.0/3090.8;
                                    } else {
                                        return 75.0/2071.4;
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 244.5f ) {
                                        if ( cl->stats.glue <= 7.5f ) {
                                            return 57.0/4140.8;
                                        } else {
                                            return 55.0/2144.7;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.505282580853f ) {
                                            return 73.0/3239.4;
                                        } else {
                                            return 114.0/2333.9;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 5942.0f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.0729056149721f ) {
                                    if ( cl->stats.size_rel <= 0.084427639842f ) {
                                        if ( cl->stats.glue_rel_long <= 0.289139688015f ) {
                                            return 86.0/3088.8;
                                        } else {
                                            return 89.0/1979.8;
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0142546016723f ) {
                                            return 128.0/2333.9;
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.0901097357273f ) {
                                                return 116.0/3326.9;
                                            } else {
                                                return 88.0/2024.6;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->size() <= 6.5f ) {
                                        if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                            return 22.0/2266.7;
                                        } else {
                                            return 67.0/3607.7;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 517.5f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0865046754479f ) {
                                                return 35.0/2187.4;
                                            } else {
                                                return 58.0/2403.1;
                                            }
                                        } else {
                                            if ( cl->stats.dump_number <= 12.5f ) {
                                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.0331632643938f ) {
                                                    return 64.0/2417.3;
                                                } else {
                                                    return 91.0/2022.6;
                                                }
                                            } else {
                                                return 129.0/2871.1;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 17.5f ) {
                                    if ( cl->stats.size_rel <= 0.260862648487f ) {
                                        return 133.0/2065.3;
                                    } else {
                                        return 188.0/2291.2;
                                    }
                                } else {
                                    return 137.0/2989.1;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_total_lits_antecedents <= 23.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 7346.5f ) {
                            if ( cl->stats.used_for_uip_creation <= 9.5f ) {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.090410426259f ) {
                                        return 241.0/1729.6;
                                    } else {
                                        return 380.0/3208.9;
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 0.254853069782f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0279234051704f ) {
                                            return 284.0/3129.5;
                                        } else {
                                            return 198.0/3170.2;
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 5184.0f ) {
                                            return 87.0/2036.8;
                                        } else {
                                            return 137.0/2185.4;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.0524673238397f ) {
                                    return 153.0/2002.2;
                                } else {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 12.5f ) {
                                        return 160.0/2869.0;
                                    } else {
                                        return 115.0/3394.0;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                if ( rdb0_last_touched_diff <= 6810.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0836919844151f ) {
                                        return 324.0/1558.6;
                                    } else {
                                        return 240.0/1751.9;
                                    }
                                } else {
                                    return 346.0/1402.0;
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.0682036280632f ) {
                                    return 232.0/1695.0;
                                } else {
                                    return 220.0/2403.1;
                                }
                            }
                        }
                    } else {
                        if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 6.5f ) {
                                return 500.0/3066.4;
                            } else {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 38.5f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.478794634342f ) {
                                        if ( rdb0_last_touched_diff <= 5183.5f ) {
                                            return 100.0/1941.2;
                                        } else {
                                            return 163.0/1863.9;
                                        }
                                    } else {
                                        return 230.0/2213.8;
                                    }
                                } else {
                                    return 86.0/1994.1;
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 4932.0f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 11989.0f ) {
                                    if ( cl->stats.used_for_uip_creation <= 10.5f ) {
                                        return 378.0/2742.9;
                                    } else {
                                        return 136.0/2732.7;
                                    }
                                } else {
                                    return 532.0/2464.1;
                                }
                            } else {
                                if ( cl->stats.size_rel <= 0.431696206331f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.5436424613f ) {
                                        return 497.0/2545.5;
                                    } else {
                                        return 241.0/1770.3;
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 8.5f ) {
                                        return 360.0/1711.3;
                                    } else {
                                        return 498.0/1705.1;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.rdb1_last_touched_diff <= 50586.5f ) {
                if ( rdb0_last_touched_diff <= 18958.0f ) {
                    if ( cl->stats.num_total_lits_antecedents <= 21.5f ) {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                            return 561.0/1981.9;
                        } else {
                            return 230.0/2238.3;
                        }
                    } else {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                            if ( cl->stats.glue <= 6.5f ) {
                                return 704.0/1617.7;
                            } else {
                                return 1148.0/1320.6;
                            }
                        } else {
                            return 630.0/2946.4;
                        }
                    }
                } else {
                    if ( cl->stats.size_rel <= 0.658777832985f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 28.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 35580.5f ) {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.128085285425f ) {
                                        return 493.0/1302.3;
                                    } else {
                                        return 565.0/1715.3;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.522413611412f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 16.5f ) {
                                            return 727.0/1931.0;
                                        } else {
                                            return 529.0/1294.1;
                                        }
                                    } else {
                                        return 769.0/1338.9;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 6.5f ) {
                                    return 962.0/1648.2;
                                } else {
                                    return 706.0/1029.6;
                                }
                            }
                        } else {
                            if ( cl->stats.num_antecedents_rel <= 0.0730937421322f ) {
                                return 972.0/716.2;
                            } else {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.0573456808925f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.364402294159f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 26018.5f ) {
                                            return 476.0/1127.3;
                                        } else {
                                            return 651.0/797.6;
                                        }
                                    } else {
                                        return 473.0/1224.9;
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 86.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.249257862568f ) {
                                            return 1054.0/1267.7;
                                        } else {
                                            return 569.0/1115.1;
                                        }
                                    } else {
                                        return 1354.0/1194.4;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                            return 1365.0/1448.8;
                        } else {
                            if ( cl->stats.dump_number <= 4.5f ) {
                                return 1336.0/626.7;
                            } else {
                                return 1160.0/962.5;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.size_rel <= 0.496999979019f ) {
                    if ( cl->stats.size_rel <= 0.262242764235f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 97499.5f ) {
                            if ( cl->stats.glue_rel_long <= 0.581405997276f ) {
                                if ( rdb0_last_touched_diff <= 88359.0f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0563029646873f ) {
                                        return 651.0/885.1;
                                    } else {
                                        return 924.0/1617.7;
                                    }
                                } else {
                                    return 812.0/1049.9;
                                }
                            } else {
                                return 769.0/685.7;
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 20.5f ) {
                                if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                    return 1097.0/1306.3;
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.0860482901335f ) {
                                        return 1116.0/1068.3;
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0400956422091f ) {
                                            return 854.0/535.1;
                                        } else {
                                            return 730.0/636.9;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 195820.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.550244450569f ) {
                                        return 1189.0/1092.7;
                                    } else {
                                        return 849.0/504.6;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 14.5f ) {
                                        return 935.0/459.9;
                                    } else {
                                        return 1368.0/854.6;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->size() <= 9.5f ) {
                            if ( cl->size() <= 6.5f ) {
                                return 763.0/1070.3;
                            } else {
                                if ( cl->stats.size_rel <= 0.422634959221f ) {
                                    if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                        return 775.0/771.2;
                                    } else {
                                        if ( rdb0_last_touched_diff <= 183969.5f ) {
                                            return 890.0/720.3;
                                        } else {
                                            return 843.0/402.9;
                                        }
                                    }
                                } else {
                                    return 728.0/927.9;
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 139056.0f ) {
                                if ( cl->stats.glue_rel_queue <= 0.578959405422f ) {
                                    return 1381.0/1168.0;
                                } else {
                                    return 1436.0/781.4;
                                }
                            } else {
                                if ( cl->stats.dump_number <= 59.5f ) {
                                    if ( rdb0_last_touched_diff <= 292416.0f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.18795453012f ) {
                                            return 1363.0/468.0;
                                        } else {
                                            return 1026.0/429.3;
                                        }
                                    } else {
                                        return 960.0/213.7;
                                    }
                                } else {
                                    return 826.0/417.1;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_total_lits_antecedents <= 29.5f ) {
                        if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                            return 1280.0/940.1;
                        } else {
                            if ( cl->stats.num_overlap_literals_rel <= 0.113295532763f ) {
                                return 1239.0/425.3;
                            } else {
                                return 881.0/425.3;
                            }
                        }
                    } else {
                        if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                            if ( cl->size() <= 18.5f ) {
                                if ( cl->stats.size_rel <= 0.634422540665f ) {
                                    return 874.0/618.6;
                                } else {
                                    return 1244.0/614.5;
                                }
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.393266439438f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.222784817219f ) {
                                        return 897.0/299.1;
                                    } else {
                                        if ( cl->size() <= 33.5f ) {
                                            return 880.0/476.1;
                                        } else {
                                            return 873.0/394.7;
                                        }
                                    }
                                } else {
                                    if ( cl->size() <= 33.5f ) {
                                        return 1209.0/480.2;
                                    } else {
                                        return 947.0/256.4;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.603903114796f ) {
                                if ( rdb0_last_touched_diff <= 128715.0f ) {
                                    return 809.0/474.1;
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.47417986393f ) {
                                        return 895.0/327.6;
                                    } else {
                                        return 1547.0/457.8;
                                    }
                                }
                            } else {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.017093937844f ) {
                                    return 1487.0/482.2;
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 48.5f ) {
                                        return 1414.0/455.8;
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 3.41333341599f ) {
                                            if ( cl->size() <= 22.5f ) {
                                                return 968.0/195.3;
                                            } else {
                                                return 1128.0/154.6;
                                            }
                                        } else {
                                            if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                                return 1492.0/404.9;
                                            } else {
                                                return 941.0/166.9;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    } else {
        if ( cl->stats.used_for_uip_creation <= 0.5f ) {
            if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                if ( cl->stats.dump_number <= 12.5f ) {
                    if ( cl->stats.num_total_lits_antecedents <= 76.5f ) {
                        if ( rdb0_last_touched_diff <= 43086.0f ) {
                            if ( cl->stats.glue <= 9.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.397530853748f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                                        return 515.0/1261.6;
                                    } else {
                                        return 446.0/1320.6;
                                    }
                                } else {
                                    return 1046.0/1886.2;
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                    return 855.0/1340.9;
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 19.5f ) {
                                        return 747.0/866.8;
                                    } else {
                                        return 944.0/616.5;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 1.02278339863f ) {
                                return 1335.0/1100.8;
                            } else {
                                return 864.0/411.0;
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                            if ( cl->size() <= 98.5f ) {
                                if ( cl->stats.glue <= 12.5f ) {
                                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                        return 1271.0/1202.6;
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.615428209305f ) {
                                            return 932.0/341.8;
                                        } else {
                                            if ( cl->stats.glue <= 9.5f ) {
                                                return 1007.0/744.7;
                                            } else {
                                                return 1572.0/645.0;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 1.00101113319f ) {
                                            return 1081.0/777.3;
                                        } else {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.917707681656f ) {
                                                return 1277.0/404.9;
                                            } else {
                                                if ( cl->stats.glue_rel_queue <= 1.18288040161f ) {
                                                    return 1016.0/173.0;
                                                } else {
                                                    return 965.0/213.7;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 164.5f ) {
                                            return 1704.0/468.0;
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.816926002502f ) {
                                                return 1041.0/166.9;
                                            } else {
                                                if ( cl->stats.num_total_lits_antecedents <= 461.5f ) {
                                                    return 1216.0/152.6;
                                                } else {
                                                    return 1160.0/89.5;
                                                }
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.401605248451f ) {
                                    return 971.0/203.5;
                                } else {
                                    if ( cl->stats.size_rel <= 2.1118350029f ) {
                                        return 1606.0/95.6;
                                    } else {
                                        return 975.0/103.8;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                                return 941.0/750.8;
                            } else {
                                return 501.0/1054.0;
                            }
                        }
                    }
                } else {
                    if ( rdb0_last_touched_diff <= 104263.0f ) {
                        if ( cl->stats.glue <= 6.5f ) {
                            return 502.0/1607.5;
                        } else {
                            if ( cl->size() <= 11.5f ) {
                                return 478.0/1141.5;
                            } else {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.179382666945f ) {
                                            return 513.0/1086.6;
                                        } else {
                                            return 853.0/1286.0;
                                        }
                                    } else {
                                        return 410.0/1416.2;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 1.10243475437f ) {
                                        return 1121.0/1257.5;
                                    } else {
                                        return 737.0/712.2;
                                    }
                                }
                            }
                        }
                    } else {
                        return 1667.0/407.0;
                    }
                }
            } else {
                if ( cl->stats.antecedents_glue_long_reds_var <= 0.250839829445f ) {
                    if ( rdb0_last_touched_diff <= 112244.5f ) {
                        if ( rdb0_last_touched_diff <= 26046.0f ) {
                            return 694.0/1109.0;
                        } else {
                            if ( cl->stats.size_rel <= 1.22785937786f ) {
                                if ( cl->stats.glue <= 8.5f ) {
                                    return 1109.0/1302.3;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 44319.0f ) {
                                        return 784.0/718.3;
                                    } else {
                                        return 1322.0/763.0;
                                    }
                                }
                            } else {
                                return 935.0/443.6;
                            }
                        }
                    } else {
                        if ( cl->stats.antec_num_total_lits_rel <= 0.344695806503f ) {
                            if ( cl->stats.num_overlap_literals <= 6.5f ) {
                                return 991.0/504.6;
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.229225873947f ) {
                                    return 1402.0/374.4;
                                } else {
                                    return 849.0/386.6;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 230463.5f ) {
                                return 1593.0/364.2;
                            } else {
                                return 1252.0/197.4;
                            }
                        }
                    }
                } else {
                    if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                        if ( cl->stats.glue_rel_long <= 0.976877212524f ) {
                            if ( cl->stats.glue <= 7.5f ) {
                                return 1190.0/657.2;
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.982182502747f ) {
                                    if ( cl->stats.size_rel <= 0.82056748867f ) {
                                        if ( rdb0_last_touched_diff <= 70737.5f ) {
                                            return 1139.0/581.9;
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 0.890694737434f ) {
                                                return 1256.0/476.1;
                                            } else {
                                                return 1053.0/248.2;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.511053025723f ) {
                                            if ( cl->stats.rdb1_act_ranking_top_10 <= 5.5f ) {
                                                return 1194.0/284.9;
                                            } else {
                                                return 1029.0/199.4;
                                            }
                                        } else {
                                            return 1243.0/384.6;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.866585969925f ) {
                                        return 1145.0/325.6;
                                    } else {
                                        if ( cl->stats.glue <= 12.5f ) {
                                            return 1062.0/195.3;
                                        } else {
                                            return 1046.0/126.2;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 79.5f ) {
                                if ( rdb0_last_touched_diff <= 66116.0f ) {
                                    return 1408.0/700.0;
                                } else {
                                    if ( cl->stats.glue <= 10.5f ) {
                                        return 1147.0/411.0;
                                    } else {
                                        if ( cl->stats.size_rel <= 1.04120326042f ) {
                                            return 1456.0/386.6;
                                        } else {
                                            return 1466.0/236.0;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 179.5f ) {
                                    if ( cl->stats.glue_rel_long <= 1.19512224197f ) {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 9.60468769073f ) {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 3.44189357758f ) {
                                                return 978.0/325.6;
                                            } else {
                                                return 1226.0/331.7;
                                            }
                                        } else {
                                            if ( rdb0_last_touched_diff <= 151497.5f ) {
                                                return 1545.0/345.9;
                                            } else {
                                                return 1111.0/126.2;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 51385.5f ) {
                                            return 1278.0/307.3;
                                        } else {
                                            if ( cl->stats.size_rel <= 0.908914923668f ) {
                                                return 936.0/185.2;
                                            } else {
                                                if ( cl->stats.num_total_lits_antecedents <= 110.5f ) {
                                                    return 977.0/130.2;
                                                } else {
                                                    return 1859.0/148.5;
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 877.5f ) {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 11.6540699005f ) {
                                            if ( cl->stats.num_antecedents_rel <= 2.11206698418f ) {
                                                if ( rdb0_last_touched_diff <= 156774.0f ) {
                                                    if ( cl->stats.antec_num_total_lits_rel <= 1.0107293129f ) {
                                                        return 967.0/152.6;
                                                    } else {
                                                        return 1049.0/234.0;
                                                    }
                                                } else {
                                                    return 1124.0/113.9;
                                                }
                                            } else {
                                                return 924.0/244.2;
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 1.05088627338f ) {
                                                return 1826.0/268.6;
                                            } else {
                                                if ( cl->stats.num_antecedents_rel <= 2.11037635803f ) {
                                                    if ( cl->stats.rdb1_last_touched_diff <= 136701.0f ) {
                                                        if ( cl->stats.num_overlap_literals <= 188.5f ) {
                                                            if ( cl->stats.num_overlap_literals <= 142.5f ) {
                                                                return 1077.0/111.9;
                                                            } else {
                                                                return 954.0/170.9;
                                                            }
                                                        } else {
                                                            if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                                                return 1500.0/95.6;
                                                            } else {
                                                                return 1408.0/203.5;
                                                            }
                                                        }
                                                    } else {
                                                        if ( cl->stats.num_overlap_literals <= 225.5f ) {
                                                            return 1415.0/99.7;
                                                        } else {
                                                            return 1113.0/26.5;
                                                        }
                                                    }
                                                } else {
                                                    return 1034.0/168.9;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->size() <= 98.5f ) {
                                            return 1813.0/227.9;
                                        } else {
                                            if ( cl->stats.num_overlap_literals <= 934.5f ) {
                                                return 1130.0/79.4;
                                            } else {
                                                if ( rdb0_last_touched_diff <= 81851.5f ) {
                                                    return 1071.0/54.9;
                                                } else {
                                                    return 1019.0/32.6;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.size_rel <= 0.802497565746f ) {
                            if ( cl->stats.num_overlap_literals <= 41.5f ) {
                                return 902.0/274.7;
                            } else {
                                if ( rdb0_last_touched_diff <= 168968.0f ) {
                                    return 1370.0/223.8;
                                } else {
                                    return 1274.0/128.2;
                                }
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 7.72370147705f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 157892.0f ) {
                                    return 1094.0/170.9;
                                } else {
                                    return 1745.0/134.3;
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 1.13360667229f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 20.8457241058f ) {
                                        return 1020.0/85.5;
                                    } else {
                                        return 954.0/116.0;
                                    }
                                } else {
                                    if ( cl->stats.glue <= 48.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 1.36185669899f ) {
                                            return 1912.0/44.8;
                                        } else {
                                            return 1346.0/77.3;
                                        }
                                    } else {
                                        return 1054.0/89.5;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.rdb1_last_touched_diff <= 13609.5f ) {
                if ( cl->stats.antecedents_glue_long_reds_var <= 0.289991676807f ) {
                    if ( cl->stats.num_antecedents_rel <= 0.10341745615f ) {
                        if ( cl->stats.antec_num_total_lits_rel <= 0.0510894954205f ) {
                            return 164.0/2496.7;
                        } else {
                            return 327.0/3082.7;
                        }
                    } else {
                        if ( cl->size() <= 8.5f ) {
                            return 126.0/2508.9;
                        } else {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 4.5f ) {
                                if ( rdb0_last_touched_diff <= 2696.5f ) {
                                    return 188.0/1898.5;
                                } else {
                                    return 294.0/1499.6;
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 1770.0f ) {
                                    return 69.0/4059.4;
                                } else {
                                    return 149.0/2181.3;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 8.5f ) {
                        if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 167.5f ) {
                                if ( rdb0_last_touched_diff <= 4931.5f ) {
                                    if ( rdb0_last_touched_diff <= 1745.5f ) {
                                        return 166.0/2016.5;
                                    } else {
                                        return 229.0/1762.1;
                                    }
                                } else {
                                    return 406.0/1503.7;
                                }
                            } else {
                                return 525.0/1941.2;
                            }
                        } else {
                            return 427.0/1542.4;
                        }
                    } else {
                        if ( cl->stats.num_antecedents_rel <= 0.209873080254f ) {
                            return 114.0/2240.3;
                        } else {
                            return 110.0/2875.1;
                        }
                    }
                }
            } else {
                if ( cl->stats.num_total_lits_antecedents <= 90.5f ) {
                    if ( cl->stats.rdb1_last_touched_diff <= 47295.0f ) {
                        if ( cl->stats.antec_num_total_lits_rel <= 0.214234828949f ) {
                            return 311.0/1731.6;
                        } else {
                            return 391.0/1664.5;
                        }
                    } else {
                        return 527.0/1471.1;
                    }
                } else {
                    if ( cl->stats.glue_rel_queue <= 1.02925515175f ) {
                        return 494.0/1601.4;
                    } else {
                        return 653.0/1615.6;
                    }
                }
            }
        }
    }
}

static double estimator_should_keep_short_conf2_cluster0_9(
    const CMSat::Clause* cl
    , const uint64_t sumConflicts
    , const uint32_t rdb0_last_touched_diff
    , const uint32_t rdb0_act_ranking
    , const uint32_t rdb0_act_ranking_top_10
) {

    double rdb_rel_used_for_uip_creation = 0;
    if (cl->stats.used_for_uip_creation > cl->stats.rdb1_used_for_uip_creation) {
        rdb_rel_used_for_uip_creation = 1.0;
    } else {
        rdb_rel_used_for_uip_creation = 0.0;
    }

    double rdb0_avg_confl;
    if (cl->stats.sum_delta_confl_uip1_used == 0) {
        rdb0_avg_confl = 0;
    } else {
        rdb0_avg_confl = ((double)cl->stats.sum_uip1_used)/((double)cl->stats.sum_delta_confl_uip1_used);
    }

    double rdb0_used_per_confl;
    if (sumConflicts-cl->stats.introduced_at_conflict == 0) {
        rdb0_used_per_confl = 0;
    } else {
        rdb0_used_per_confl = ((double)cl->stats.sum_uip1_used)/((double)sumConflicts-(double)cl->stats.introduced_at_conflict);
    }
    if ( rdb0_last_touched_diff <= 23636.5f ) {
        if ( cl->stats.antec_num_total_lits_rel <= 0.50461345911f ) {
            if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                if ( rdb0_last_touched_diff <= 9618.5f ) {
                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.0964741259813f ) {
                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                            if ( rdb0_last_touched_diff <= 1815.5f ) {
                                if ( cl->size() <= 10.5f ) {
                                    if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                                            return 402.0/2639.1;
                                        } else {
                                            return 213.0/2071.4;
                                        }
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.0399368591607f ) {
                                            return 179.0/1998.2;
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 0.237910196185f ) {
                                                return 155.0/2368.5;
                                            } else {
                                                if ( cl->stats.glue_rel_queue <= 0.607041835785f ) {
                                                    if ( cl->stats.num_total_lits_antecedents <= 9.5f ) {
                                                        return 103.0/2061.2;
                                                    } else {
                                                        if ( rdb0_last_touched_diff <= 726.0f ) {
                                                            if ( cl->stats.glue_rel_queue <= 0.406835079193f ) {
                                                                return 73.0/2309.5;
                                                            } else {
                                                                return 58.0/2407.1;
                                                            }
                                                        } else {
                                                            return 164.0/3086.8;
                                                        }
                                                    }
                                                } else {
                                                    return 137.0/2144.7;
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.0732569992542f ) {
                                        return 233.0/1692.9;
                                    } else {
                                        if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                            return 318.0/1788.6;
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 15019.0f ) {
                                                return 149.0/2962.6;
                                            } else {
                                                return 114.0/1819.1;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 4695.0f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.643579602242f ) {
                                            if ( cl->size() <= 4.5f ) {
                                                return 196.0/1715.3;
                                            } else {
                                                if ( cl->stats.size_rel <= 0.203069657087f ) {
                                                    return 312.0/1444.7;
                                                } else {
                                                    if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                                        return 412.0/1817.1;
                                                    } else {
                                                        return 164.0/1847.6;
                                                    }
                                                }
                                            }
                                        } else {
                                            return 430.0/1988.0;
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0302464403212f ) {
                                            return 398.0/2881.3;
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 21.5f ) {
                                                return 238.0/3648.4;
                                            } else {
                                                return 286.0/2222.0;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0316827185452f ) {
                                                return 373.0/2169.1;
                                            } else {
                                                if ( cl->size() <= 7.5f ) {
                                                    return 342.0/1888.3;
                                                } else {
                                                    return 360.0/1530.2;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 0.42832750082f ) {
                                                return 244.0/1595.3;
                                            } else {
                                                return 224.0/1731.6;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 42944.0f ) {
                                            return 494.0/2415.3;
                                        } else {
                                            return 460.0/1233.1;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 25.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.104335606098f ) {
                                    return 355.0/1424.3;
                                } else {
                                    return 299.0/1888.3;
                                }
                            } else {
                                return 697.0/2431.6;
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 2234.0f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 70.5f ) {
                                    if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0979321971536f ) {
                                            return 322.0/1821.1;
                                        } else {
                                            return 209.0/1788.6;
                                        }
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.122489891946f ) {
                                            return 130.0/2034.8;
                                        } else {
                                            return 124.0/2883.3;
                                        }
                                    }
                                } else {
                                    return 291.0/2097.9;
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 773.5f ) {
                                    if ( rdb0_last_touched_diff <= 241.5f ) {
                                        return 125.0/1821.1;
                                    } else {
                                        return 257.0/1990.0;
                                    }
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                        return 449.0/1456.9;
                                    } else {
                                        return 227.0/1835.4;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 12687.0f ) {
                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                        if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                            return 574.0/2342.0;
                                        } else {
                                            return 407.0/2533.3;
                                        }
                                    } else {
                                        if ( cl->size() <= 10.5f ) {
                                            return 198.0/1843.5;
                                        } else {
                                            return 415.0/2189.4;
                                        }
                                    }
                                } else {
                                    return 492.0/1589.2;
                                }
                            } else {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                                    if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                        if ( cl->stats.num_overlap_literals <= 27.5f ) {
                                            if ( cl->stats.glue_rel_queue <= 0.656491219997f ) {
                                                return 522.0/1904.6;
                                            } else {
                                                return 458.0/1104.9;
                                            }
                                        } else {
                                            return 544.0/997.0;
                                        }
                                    } else {
                                        return 379.0/2364.4;
                                    }
                                } else {
                                    return 931.0/1772.3;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_total_lits_antecedents <= 44.5f ) {
                        if ( cl->stats.size_rel <= 0.656350493431f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 22.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.0455218218267f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 6980.5f ) {
                                        return 404.0/1273.8;
                                    } else {
                                        return 462.0/1271.7;
                                    }
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                            if ( rdb0_last_touched_diff <= 16395.5f ) {
                                                return 664.0/2165.0;
                                            } else {
                                                return 648.0/2791.7;
                                            }
                                        } else {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0474189929664f ) {
                                                return 351.0/1524.1;
                                            } else {
                                                return 276.0/1664.5;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.346087306738f ) {
                                            return 663.0/1988.0;
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 0.660272240639f ) {
                                                if ( cl->stats.glue <= 4.5f ) {
                                                    return 453.0/1511.8;
                                                } else {
                                                    return 539.0/2250.5;
                                                }
                                            } else {
                                                return 421.0/1253.4;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                        if ( cl->stats.num_overlap_literals <= 14.5f ) {
                                            return 664.0/1658.3;
                                        } else {
                                            return 436.0/1542.4;
                                        }
                                    } else {
                                        return 478.0/1865.9;
                                    }
                                } else {
                                    if ( cl->size() <= 10.5f ) {
                                        if ( cl->stats.dump_number <= 4.5f ) {
                                            return 458.0/1176.1;
                                        } else {
                                            return 406.0/1253.4;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.63296943903f ) {
                                            return 586.0/1141.5;
                                        } else {
                                            return 611.0/840.4;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_antecedents_rel <= 0.198986947536f ) {
                                return 678.0/1265.6;
                            } else {
                                return 790.0/1060.1;
                            }
                        }
                    } else {
                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                            if ( cl->stats.size_rel <= 0.605286836624f ) {
                                if ( cl->size() <= 18.5f ) {
                                    return 622.0/1650.2;
                                } else {
                                    return 571.0/1049.9;
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.821428120136f ) {
                                    return 610.0/999.1;
                                } else {
                                    return 626.0/879.0;
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.684324264526f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 8628.0f ) {
                                    return 864.0/1100.8;
                                } else {
                                    return 570.0/1001.1;
                                }
                            } else {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.331111133099f ) {
                                    return 875.0/1172.0;
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                                        return 807.0/598.2;
                                    } else {
                                        return 1355.0/494.5;
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.rdb1_last_touched_diff <= 2101.5f ) {
                    if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 10.5f ) {
                            if ( rdb0_last_touched_diff <= 9125.0f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.0555055178702f ) {
                                    return 246.0/1845.5;
                                } else {
                                    if ( rdb0_last_touched_diff <= 3669.5f ) {
                                        if ( rdb0_last_touched_diff <= 798.5f ) {
                                            return 92.0/1941.2;
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 20.5f ) {
                                                return 105.0/2024.6;
                                            } else {
                                                return 192.0/2016.5;
                                            }
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 6635.5f ) {
                                            return 215.0/2517.0;
                                        } else {
                                            return 204.0/1819.1;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 38.5f ) {
                                    if ( cl->stats.num_overlap_literals <= 6.5f ) {
                                        return 284.0/2142.6;
                                    } else {
                                        return 339.0/1754.0;
                                    }
                                } else {
                                    return 418.0/1389.8;
                                }
                            }
                        } else {
                            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 1095.5f ) {
                                    if ( cl->stats.num_overlap_literals <= 15.5f ) {
                                        if ( cl->stats.size_rel <= 0.0340048335493f ) {
                                            return 34.0/2059.2;
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 0.183279812336f ) {
                                                return 102.0/1971.7;
                                            } else {
                                                if ( cl->stats.glue_rel_long <= 0.261891901493f ) {
                                                    return 115.0/3149.8;
                                                } else {
                                                    if ( rdb0_last_touched_diff <= 7118.0f ) {
                                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0423841252923f ) {
                                                            return 93.0/3632.1;
                                                        } else {
                                                            if ( cl->stats.size_rel <= 0.283390492201f ) {
                                                                if ( cl->stats.antec_num_total_lits_rel <= 0.0680276229978f ) {
                                                                    return 44.0/2120.2;
                                                                } else {
                                                                    return 50.0/4053.3;
                                                                }
                                                            } else {
                                                                if ( rdb0_last_touched_diff <= 1279.0f ) {
                                                                    return 41.0/3573.1;
                                                                } else {
                                                                    return 103.0/2604.5;
                                                                }
                                                            }
                                                        }
                                                    } else {
                                                        return 158.0/1845.5;
                                                    }
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.196721225977f ) {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 43.5f ) {
                                                return 150.0/1894.4;
                                            } else {
                                                return 62.0/2049.0;
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 40.5f ) {
                                                return 92.0/2150.8;
                                            } else {
                                                return 36.0/2502.8;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 7.5f ) {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 19.5f ) {
                                            return 219.0/2293.2;
                                        } else {
                                            return 130.0/1859.8;
                                        }
                                    } else {
                                        if ( cl->stats.size_rel <= 0.201955035329f ) {
                                            return 69.0/2401.0;
                                        } else {
                                            return 55.0/2887.4;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.537746310234f ) {
                                    return 197.0/3141.7;
                                } else {
                                    return 151.0/1857.8;
                                }
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 3879.0f ) {
                            if ( cl->stats.glue_rel_long <= 0.550695419312f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 898.0f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 12.5f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 23.5f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 353.5f ) {
                                                return 42.0/2289.1;
                                            } else {
                                                return 50.0/2106.0;
                                            }
                                        } else {
                                            return 84.0/2266.7;
                                        }
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.1264346838f ) {
                                            if ( cl->stats.dump_number <= 9.5f ) {
                                                return 84.0/3945.4;
                                            } else {
                                                if ( cl->stats.rdb1_last_touched_diff <= 148.5f ) {
                                                    return 19.0/2083.6;
                                                } else {
                                                    return 37.0/2256.6;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 0.348185509443f ) {
                                                return 37.0/2541.4;
                                            } else {
                                                return 9.0/2549.6;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 19.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0270927511156f ) {
                                            return 50.0/1975.8;
                                        } else {
                                            return 27.0/2272.9;
                                        }
                                    } else {
                                        return 130.0/3326.9;
                                    }
                                }
                            } else {
                                if ( cl->stats.size_rel <= 0.407514691353f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 14.5f ) {
                                        return 89.0/2700.2;
                                    } else {
                                        return 45.0/3265.8;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 425.5f ) {
                                        return 67.0/2826.3;
                                    } else {
                                        return 112.0/2704.2;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals <= 7.5f ) {
                                return 152.0/1949.3;
                            } else {
                                return 181.0/1802.8;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 57.5f ) {
                            if ( cl->stats.glue_rel_queue <= 0.648531794548f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0182908512652f ) {
                                    return 391.0/1589.2;
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.102364853024f ) {
                                        return 380.0/1886.2;
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.176431328058f ) {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.105613462627f ) {
                                                return 239.0/1560.7;
                                            } else {
                                                return 262.0/2034.8;
                                            }
                                        } else {
                                            return 265.0/1583.1;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 4660.0f ) {
                                    return 288.0/1465.0;
                                } else {
                                    return 377.0/1434.5;
                                }
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.853571414948f ) {
                                return 362.0/1481.3;
                            } else {
                                return 506.0/1052.0;
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 11.5f ) {
                            if ( rdb0_last_touched_diff <= 4361.5f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.0327036678791f ) {
                                    return 255.0/2653.4;
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 7.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 6570.5f ) {
                                            if ( cl->stats.glue <= 5.5f ) {
                                                return 219.0/3707.4;
                                            } else {
                                                return 284.0/3263.8;
                                            }
                                        } else {
                                            return 192.0/1770.3;
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 1837.0f ) {
                                            if ( rdb0_last_touched_diff <= 644.0f ) {
                                                return 83.0/3874.2;
                                            } else {
                                                return 94.0/2700.2;
                                            }
                                        } else {
                                            return 122.0/2309.5;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                    return 448.0/2948.4;
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.1039429456f ) {
                                        return 203.0/1776.4;
                                    } else {
                                        return 179.0/2028.7;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 7.5f ) {
                                if ( rdb0_last_touched_diff <= 1566.5f ) {
                                    return 56.0/2038.9;
                                } else {
                                    if ( cl->stats.size_rel <= 0.278650760651f ) {
                                        return 208.0/2169.1;
                                    } else {
                                        return 136.0/1988.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.0474635399878f ) {
                                    return 137.0/3758.2;
                                } else {
                                    if ( rdb0_last_touched_diff <= 2370.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 3912.0f ) {
                                            if ( cl->stats.num_total_lits_antecedents <= 27.5f ) {
                                                return 18.0/2101.9;
                                            } else {
                                                return 39.0/2366.5;
                                            }
                                        } else {
                                            return 77.0/2789.7;
                                        }
                                    } else {
                                        return 85.0/2346.1;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.glue <= 9.5f ) {
                if ( rdb0_last_touched_diff <= 10079.0f ) {
                    if ( cl->stats.rdb1_last_touched_diff <= 11067.0f ) {
                        if ( rdb0_last_touched_diff <= 3114.5f ) {
                            if ( cl->stats.used_for_uip_creation <= 6.5f ) {
                                return 160.0/1747.9;
                            } else {
                                return 90.0/2999.3;
                            }
                        } else {
                            return 449.0/2919.9;
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 3110.5f ) {
                            return 269.0/1648.2;
                        } else {
                            return 428.0/1279.9;
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                        if ( cl->stats.dump_number <= 2.5f ) {
                            return 991.0/1800.8;
                        } else {
                            return 636.0/2268.8;
                        }
                    } else {
                        return 866.0/1066.2;
                    }
                }
            } else {
                if ( cl->stats.rdb1_last_touched_diff <= 1556.5f ) {
                    if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                        return 932.0/759.0;
                    } else {
                        return 325.0/3522.2;
                    }
                } else {
                    if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                        if ( cl->stats.dump_number <= 2.5f ) {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                    return 1120.0/435.4;
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 556.5f ) {
                                        if ( cl->stats.glue_rel_long <= 1.07155561447f ) {
                                            return 857.0/343.9;
                                        } else {
                                            return 1590.0/284.9;
                                        }
                                    } else {
                                        return 1279.0/75.3;
                                    }
                                }
                            } else {
                                return 1037.0/748.8;
                            }
                        } else {
                            return 1121.0/1941.2;
                        }
                    } else {
                        if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                            if ( cl->stats.num_overlap_literals <= 87.5f ) {
                                return 319.0/2327.8;
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 7223.0f ) {
                                    return 277.0/1601.4;
                                } else {
                                    return 438.0/1440.6;
                                }
                            }
                        } else {
                            return 485.0/1190.3;
                        }
                    }
                }
            }
        }
    } else {
        if ( cl->stats.glue <= 9.5f ) {
            if ( rdb0_last_touched_diff <= 64339.5f ) {
                if ( cl->stats.antecedents_glue_long_reds_var <= 0.263149499893f ) {
                    if ( rdb0_last_touched_diff <= 34898.0f ) {
                        if ( cl->stats.dump_number <= 3.5f ) {
                            if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                if ( cl->stats.dump_number <= 2.5f ) {
                                    return 554.0/1052.0;
                                } else {
                                    return 510.0/1239.2;
                                }
                            } else {
                                return 722.0/1123.2;
                            }
                        } else {
                            if ( cl->stats.num_antecedents_rel <= 0.149348273873f ) {
                                if ( cl->stats.num_overlap_literals <= 3.5f ) {
                                    return 460.0/1727.5;
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 22.5f ) {
                                        return 779.0/2639.1;
                                    } else {
                                        return 512.0/1068.3;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 28.5f ) {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                        return 491.0/1666.5;
                                    } else {
                                        return 653.0/1597.3;
                                    }
                                } else {
                                    return 792.0/1548.5;
                                }
                            }
                        }
                    } else {
                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                            if ( cl->size() <= 7.5f ) {
                                return 508.0/1599.3;
                            } else {
                                return 879.0/1656.3;
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals <= 12.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 18.5f ) {
                                    if ( cl->stats.size_rel <= 0.396198660135f ) {
                                        if ( rdb0_last_touched_diff <= 47864.5f ) {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.065484136343f ) {
                                                return 571.0/1290.1;
                                            } else {
                                                return 603.0/1530.2;
                                            }
                                        } else {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.0709146335721f ) {
                                                return 694.0/1041.8;
                                            } else {
                                                return 554.0/1239.2;
                                            }
                                        }
                                    } else {
                                        return 554.0/972.6;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 7.5f ) {
                                        return 851.0/988.9;
                                    } else {
                                        if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                            return 782.0/1306.3;
                                        } else {
                                            return 663.0/832.2;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->size() <= 11.5f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.202968984842f ) {
                                        return 672.0/879.0;
                                    } else {
                                        return 651.0/1082.5;
                                    }
                                } else {
                                    return 1304.0/1084.5;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->size() <= 16.5f ) {
                        if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                            if ( cl->stats.dump_number <= 5.5f ) {
                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                    return 563.0/1188.3;
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.715016067028f ) {
                                        return 747.0/1288.0;
                                    } else {
                                        return 1164.0/1326.7;
                                    }
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                    return 653.0/1882.2;
                                } else {
                                    if ( cl->size() <= 9.5f ) {
                                        return 713.0/1587.1;
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 19.5f ) {
                                            return 682.0/911.6;
                                        } else {
                                            return 609.0/1054.0;
                                        }
                                    }
                                }
                            }
                        } else {
                            return 1082.0/824.1;
                        }
                    } else {
                        if ( cl->stats.dump_number <= 6.5f ) {
                            if ( cl->stats.glue_rel_queue <= 0.710419774055f ) {
                                return 1082.0/954.3;
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                    return 862.0/636.9;
                                } else {
                                    return 940.0/274.7;
                                }
                            }
                        } else {
                            if ( cl->stats.num_antecedents_rel <= 0.251767277718f ) {
                                return 787.0/750.8;
                            } else {
                                return 815.0/986.9;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.num_total_lits_antecedents <= 22.5f ) {
                    if ( cl->stats.num_total_lits_antecedents <= 9.5f ) {
                        return 1112.0/1796.7;
                    } else {
                        if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                            if ( cl->stats.glue_rel_queue <= 0.236792266369f ) {
                                return 870.0/811.9;
                            } else {
                                if ( rdb0_last_touched_diff <= 203231.0f ) {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.119558736682f ) {
                                            return 639.0/879.0;
                                        } else {
                                            return 606.0/972.6;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.616539239883f ) {
                                            if ( cl->stats.num_antecedents_rel <= 0.157643154263f ) {
                                                return 1005.0/1536.3;
                                            } else {
                                                return 656.0/767.1;
                                            }
                                        } else {
                                            return 850.0/885.1;
                                        }
                                    }
                                } else {
                                    return 788.0/630.8;
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.371710181236f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 255926.5f ) {
                                    return 1174.0/1206.6;
                                } else {
                                    return 867.0/461.9;
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 195570.5f ) {
                                    if ( cl->stats.size_rel <= 0.435650229454f ) {
                                        return 1409.0/1300.2;
                                    } else {
                                        return 768.0/584.0;
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.135248988867f ) {
                                        return 898.0/561.6;
                                    } else {
                                        return 1036.0/431.4;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->size() <= 9.5f ) {
                        if ( cl->size() <= 5.5f ) {
                            return 916.0/1324.6;
                        } else {
                            if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                return 940.0/1123.2;
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.732741475105f ) {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 6.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.547695219517f ) {
                                            return 754.0/620.6;
                                        } else {
                                            return 698.0/667.4;
                                        }
                                    } else {
                                        return 1348.0/881.1;
                                    }
                                } else {
                                    return 1531.0/856.6;
                                }
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 138502.5f ) {
                            if ( cl->stats.glue_rel_queue <= 0.630086660385f ) {
                                if ( cl->stats.num_overlap_literals <= 23.5f ) {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                                        return 688.0/742.7;
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.464563310146f ) {
                                            return 768.0/748.8;
                                        } else {
                                            return 945.0/577.9;
                                        }
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 91539.0f ) {
                                        return 815.0/569.7;
                                    } else {
                                        return 1010.0/539.2;
                                    }
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                    if ( cl->stats.size_rel <= 0.69682019949f ) {
                                        return 1075.0/901.4;
                                    } else {
                                        return 883.0/409.0;
                                    }
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                        if ( cl->stats.dump_number <= 12.5f ) {
                                            if ( cl->stats.glue_rel_long <= 0.794584393501f ) {
                                                return 921.0/236.0;
                                            } else {
                                                return 1112.0/429.3;
                                            }
                                        } else {
                                            return 843.0/632.8;
                                        }
                                    } else {
                                        return 1642.0/488.3;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 312119.0f ) {
                                if ( cl->stats.dump_number <= 29.5f ) {
                                    if ( cl->stats.size_rel <= 0.742280006409f ) {
                                        if ( cl->stats.glue_rel_long <= 0.759422719479f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 197399.5f ) {
                                                return 1624.0/771.2;
                                            } else {
                                                return 955.0/307.3;
                                            }
                                        } else {
                                            return 1230.0/335.7;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 208076.5f ) {
                                            return 1823.0/425.3;
                                        } else {
                                            return 965.0/181.1;
                                        }
                                    }
                                } else {
                                    if ( cl->size() <= 17.5f ) {
                                        return 1369.0/791.5;
                                    } else {
                                        return 901.0/378.5;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.233275145292f ) {
                                    if ( cl->stats.dump_number <= 60.5f ) {
                                        return 1326.0/299.1;
                                    } else {
                                        return 922.0/392.7;
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 0.842054247856f ) {
                                        return 1223.0/266.6;
                                    } else {
                                        return 1069.0/144.5;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.glue_rel_long <= 0.873259902f ) {
                if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                    if ( rdb0_last_touched_diff <= 60255.5f ) {
                        if ( cl->size() <= 58.5f ) {
                            if ( rdb0_last_touched_diff <= 35469.0f ) {
                                if ( cl->stats.size_rel <= 0.70502102375f ) {
                                    return 842.0/1544.4;
                                } else {
                                    return 660.0/901.4;
                                }
                            } else {
                                if ( cl->size() <= 23.5f ) {
                                    return 863.0/1178.1;
                                } else {
                                    return 996.0/909.5;
                                }
                            }
                        } else {
                            return 742.0/551.4;
                        }
                    } else {
                        if ( cl->size() <= 37.5f ) {
                            if ( rdb0_last_touched_diff <= 96847.0f ) {
                                return 1261.0/1021.5;
                            } else {
                                return 843.0/439.5;
                            }
                        } else {
                            return 1259.0/413.1;
                        }
                    }
                } else {
                    if ( rdb0_last_touched_diff <= 105411.5f ) {
                        if ( cl->stats.dump_number <= 10.5f ) {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.397082149982f ) {
                                if ( cl->stats.glue_rel_queue <= 0.710840344429f ) {
                                    return 1353.0/1397.9;
                                } else {
                                    return 1116.0/569.7;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 152.5f ) {
                                    return 1037.0/352.0;
                                } else {
                                    return 976.0/152.6;
                                }
                            }
                        } else {
                            return 973.0/1239.2;
                        }
                    } else {
                        if ( cl->stats.glue_rel_long <= 0.58932697773f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.0126744098961f ) {
                                return 1161.0/773.2;
                            } else {
                                return 1273.0/431.4;
                            }
                        } else {
                            if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                return 1519.0/579.9;
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 101.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.733370184898f ) {
                                        return 1593.0/575.8;
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.271697610617f ) {
                                            return 894.0/278.8;
                                        } else {
                                            return 1382.0/307.3;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue <= 13.5f ) {
                                        return 1236.0/291.0;
                                    } else {
                                        return 1538.0/278.8;
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.num_overlap_literals <= 48.5f ) {
                    if ( cl->stats.num_overlap_literals <= 19.5f ) {
                        if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.214554071426f ) {
                                return 740.0/716.2;
                            } else {
                                return 1092.0/775.3;
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 45525.0f ) {
                                return 971.0/795.6;
                            } else {
                                if ( rdb0_last_touched_diff <= 112622.0f ) {
                                    return 1491.0/647.1;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 274730.0f ) {
                                        if ( cl->stats.size_rel <= 0.944675564766f ) {
                                            return 1198.0/427.3;
                                        } else {
                                            return 1364.0/303.2;
                                        }
                                    } else {
                                        return 1307.0/217.7;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_queue <= 1.02214920521f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 64959.5f ) {
                                return 1371.0/966.5;
                            } else {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 5.5f ) {
                                    return 1166.0/331.7;
                                } else {
                                    return 1773.0/303.2;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                                if ( cl->stats.size_rel <= 0.967329621315f ) {
                                    return 842.0/455.8;
                                } else {
                                    return 1076.0/354.1;
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                    if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.423390328884f ) {
                                            return 1417.0/254.3;
                                        } else {
                                            return 1175.0/175.0;
                                        }
                                    } else {
                                        return 1565.0/423.2;
                                    }
                                } else {
                                    return 1684.0/223.8;
                                }
                            }
                        }
                    }
                } else {
                    if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                        if ( cl->stats.dump_number <= 13.5f ) {
                            if ( cl->stats.glue <= 19.5f ) {
                                if ( cl->stats.num_overlap_literals <= 88.5f ) {
                                    return 973.0/376.4;
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                        return 1092.0/413.1;
                                    } else {
                                        return 1386.0/299.1;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 1.09942233562f ) {
                                    return 1116.0/278.8;
                                } else {
                                    return 1875.0/160.7;
                                }
                            }
                        } else {
                            return 736.0/667.4;
                        }
                    } else {
                        if ( cl->stats.glue <= 13.5f ) {
                            if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                if ( cl->stats.glue_rel_long <= 1.07959794998f ) {
                                    if ( rdb0_last_touched_diff <= 67557.5f ) {
                                        return 1100.0/400.9;
                                    } else {
                                        if ( cl->stats.rdb1_act_ranking_top_10 <= 4.5f ) {
                                            return 1130.0/170.9;
                                        } else {
                                            return 1636.0/390.7;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 1.27287435532f ) {
                                        if ( rdb0_last_touched_diff <= 96313.0f ) {
                                            return 962.0/217.7;
                                        } else {
                                            return 1181.0/111.9;
                                        }
                                    } else {
                                        return 1652.0/386.6;
                                    }
                                }
                            } else {
                                return 1801.0/164.8;
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.808428108692f ) {
                                return 1791.0/575.8;
                            } else {
                                if ( cl->stats.glue_rel_long <= 1.3471339941f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.820955634117f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.736849308014f ) {
                                            if ( cl->stats.num_total_lits_antecedents <= 197.5f ) {
                                                if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                                    return 1422.0/303.2;
                                                } else {
                                                    return 1363.0/205.5;
                                                }
                                            } else {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.667100787163f ) {
                                                    if ( rdb0_last_touched_diff <= 94936.0f ) {
                                                        return 999.0/101.7;
                                                    } else {
                                                        return 1167.0/61.0;
                                                    }
                                                } else {
                                                    return 930.0/152.6;
                                                }
                                            }
                                        } else {
                                            return 1104.0/242.1;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 351941.0f ) {
                                            if ( rdb0_last_touched_diff <= 133428.5f ) {
                                                if ( cl->stats.dump_number <= 10.5f ) {
                                                    if ( cl->stats.num_total_lits_antecedents <= 472.5f ) {
                                                        if ( cl->size() <= 24.5f ) {
                                                            return 982.0/97.7;
                                                        } else {
                                                            return 1629.0/211.6;
                                                        }
                                                    } else {
                                                        if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                                            return 1093.0/40.7;
                                                        } else {
                                                            return 1827.0/175.0;
                                                        }
                                                    }
                                                } else {
                                                    return 1173.0/242.1;
                                                }
                                            } else {
                                                if ( cl->stats.glue_rel_long <= 1.16284430027f ) {
                                                    if ( cl->stats.glue_rel_long <= 1.0834338665f ) {
                                                        return 1644.0/105.8;
                                                    } else {
                                                        return 1040.0/175.0;
                                                    }
                                                } else {
                                                    return 1881.0/81.4;
                                                }
                                            }
                                        } else {
                                            return 1067.0/195.3;
                                        }
                                    }
                                } else {
                                    if ( cl->size() <= 126.5f ) {
                                        if ( rdb0_last_touched_diff <= 64223.0f ) {
                                            if ( cl->stats.antec_num_total_lits_rel <= 1.23007082939f ) {
                                                return 1019.0/168.9;
                                            } else {
                                                return 1202.0/130.2;
                                            }
                                        } else {
                                            if ( cl->stats.num_overlap_literals_rel <= 1.03920578957f ) {
                                                if ( cl->stats.dump_number <= 18.5f ) {
                                                    return 1364.0/99.7;
                                                } else {
                                                    return 1014.0/136.3;
                                                }
                                            } else {
                                                if ( cl->stats.num_overlap_literals <= 212.5f ) {
                                                    if ( cl->size() <= 33.5f ) {
                                                        return 1014.0/46.8;
                                                    } else {
                                                        return 996.0/12.2;
                                                    }
                                                } else {
                                                    if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                                        return 1153.0/69.2;
                                                    } else {
                                                        return 1251.0/113.9;
                                                    }
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 726.5f ) {
                                            return 1212.0/95.6;
                                        } else {
                                            return 1979.0/28.5;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

static bool should_keep_short_conf2_cluster0(
    const CMSat::Clause* cl
    , const uint64_t sumConflicts
    , const uint32_t rdb0_last_touched_diff
    , const uint32_t rdb0_act_ranking
    , const uint32_t rdb0_act_ranking_top_10
) {
    int votes = 0;
    votes += estimator_should_keep_short_conf2_cluster0_0(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_short_conf2_cluster0_1(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_short_conf2_cluster0_2(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_short_conf2_cluster0_3(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_short_conf2_cluster0_4(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_short_conf2_cluster0_5(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_short_conf2_cluster0_6(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_short_conf2_cluster0_7(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_short_conf2_cluster0_8(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_short_conf2_cluster0_9(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    return votes >= 5;
}
}
