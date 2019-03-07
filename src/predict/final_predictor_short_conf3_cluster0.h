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

static double estimator_should_keep_short_conf3_cluster0_0(
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
    if ( rdb0_last_touched_diff <= 26900.5f ) {
        if ( cl->stats.used_for_uip_creation <= 0.5f ) {
            if ( cl->stats.antecedents_glue_long_reds_var <= 6.25172996521f ) {
                if ( cl->stats.size_rel <= 0.78383731842f ) {
                    if ( cl->stats.glue_rel_long <= 0.737697601318f ) {
                        if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                            if ( cl->size() <= 9.5f ) {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                    if ( cl->size() <= 8.5f ) {
                                        if ( cl->stats.glue_rel_long <= 0.3160341084f ) {
                                            if ( cl->stats.glue_rel_long <= 0.236664772034f ) {
                                                return 471.0/1408.1;
                                            } else {
                                                return 408.0/1444.4;
                                            }
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.279114484787f ) {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.120804533362f ) {
                                                    if ( cl->stats.glue_rel_queue <= 0.429491996765f ) {
                                                        return 352.0/1714.7;
                                                    } else {
                                                        return 603.0/2223.1;
                                                    }
                                                } else {
                                                    return 367.0/2144.4;
                                                }
                                            } else {
                                                return 367.0/1404.1;
                                            }
                                        }
                                    } else {
                                        return 450.0/1482.7;
                                    }
                                } else {
                                    if ( cl->size() <= 6.5f ) {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 10.5f ) {
                                            if ( cl->stats.size_rel <= 0.207563459873f ) {
                                                if ( rdb0_last_touched_diff <= 14353.0f ) {
                                                    return 464.0/2727.4;
                                                } else {
                                                    return 352.0/1502.9;
                                                }
                                            } else {
                                                return 310.0/2332.0;
                                            }
                                        } else {
                                            return 280.0/2394.6;
                                        }
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.0249307472259f ) {
                                            if ( cl->stats.size_rel <= 0.34663683176f ) {
                                                return 455.0/2289.7;
                                            } else {
                                                return 235.0/1670.3;
                                            }
                                        } else {
                                            return 451.0/2190.8;
                                        }
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 13152.5f ) {
                                    if ( cl->size() <= 13.5f ) {
                                        return 580.0/2715.3;
                                    } else {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                                            return 571.0/1436.3;
                                        } else {
                                            return 353.0/1968.9;
                                        }
                                    }
                                } else {
                                    if ( cl->size() <= 14.5f ) {
                                        if ( cl->size() <= 10.5f ) {
                                            return 468.0/1724.8;
                                        } else {
                                            if ( cl->stats.size_rel <= 0.48329693079f ) {
                                                return 743.0/2063.7;
                                            } else {
                                                return 486.0/1761.1;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 10.5f ) {
                                            return 504.0/1535.2;
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 8669.5f ) {
                                                return 645.0/1775.2;
                                            } else {
                                                return 529.0/1145.8;
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.dump_number <= 2.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 45.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.473922491074f ) {
                                        return 493.0/1591.7;
                                    } else {
                                        return 520.0/1214.4;
                                    }
                                } else {
                                    return 677.0/1002.6;
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 24.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.105052061379f ) {
                                        return 697.0/2727.4;
                                    } else {
                                        return 351.0/1773.2;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.416473925114f ) {
                                        return 413.0/1341.5;
                                    } else {
                                        return 758.0/1886.2;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->size() <= 11.5f ) {
                            if ( cl->stats.num_overlap_literals_rel <= 0.400104880333f ) {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 21.5f ) {
                                        return 459.0/2047.6;
                                    } else {
                                        return 566.0/1908.4;
                                    }
                                } else {
                                    return 522.0/1238.6;
                                }
                            } else {
                                return 473.0/1117.6;
                            }
                        } else {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                    if ( cl->stats.num_overlap_literals <= 27.5f ) {
                                        return 576.0/1297.1;
                                    } else {
                                        return 656.0/944.1;
                                    }
                                } else {
                                    return 782.0/2533.8;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.194803655148f ) {
                                    return 696.0/1143.8;
                                } else {
                                    return 669.0/752.5;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_total_lits_antecedents <= 197.5f ) {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                            if ( cl->stats.dump_number <= 2.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.543083906174f ) {
                                    return 703.0/641.5;
                                } else {
                                    return 1143.0/744.4;
                                }
                            } else {
                                if ( cl->stats.glue <= 9.5f ) {
                                    return 553.0/1281.0;
                                } else {
                                    return 680.0/1289.1;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                                return 657.0/1551.3;
                            } else {
                                return 456.0/1523.1;
                            }
                        }
                    } else {
                        return 1394.0/1107.5;
                    }
                }
            } else {
                if ( cl->stats.glue_rel_queue <= 0.957969069481f ) {
                    if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                        if ( cl->stats.num_overlap_literals <= 44.5f ) {
                            return 899.0/2045.6;
                        } else {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.936546683311f ) {
                                return 867.0/1480.7;
                            } else {
                                return 632.0/921.9;
                            }
                        }
                    } else {
                        return 1278.0/1155.9;
                    }
                } else {
                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                        if ( cl->stats.antec_num_total_lits_rel <= 0.857565164566f ) {
                            return 889.0/1075.2;
                        } else {
                            return 1031.0/625.4;
                        }
                    } else {
                        if ( cl->stats.num_overlap_literals <= 379.5f ) {
                            if ( cl->stats.glue <= 14.5f ) {
                                return 1500.0/1010.7;
                            } else {
                                if ( cl->stats.glue_rel_long <= 1.35786306858f ) {
                                    return 1611.0/542.7;
                                } else {
                                    return 1167.0/232.0;
                                }
                            }
                        } else {
                            return 1695.0/201.7;
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                if ( cl->stats.rdb1_last_touched_diff <= 18536.5f ) {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                        if ( cl->stats.glue_rel_queue <= 0.788739204407f ) {
                            if ( cl->stats.num_overlap_literals <= 8.5f ) {
                                if ( cl->size() <= 10.5f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0632392466068f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 6513.0f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0295347347856f ) {
                                                return 242.0/1888.2;
                                            } else {
                                                return 185.0/1882.2;
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 0.318527698517f ) {
                                                return 284.0/1517.0;
                                            } else {
                                                return 396.0/2878.7;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.148676812649f ) {
                                            return 151.0/1765.2;
                                        } else {
                                            return 184.0/1833.7;
                                        }
                                    }
                                } else {
                                    return 491.0/2834.3;
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 76.5f ) {
                                    if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.106569394469f ) {
                                            return 413.0/1690.5;
                                        } else {
                                            return 451.0/2404.6;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.551927149296f ) {
                                            return 345.0/2560.0;
                                        } else {
                                            return 293.0/1898.3;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 1.14087295532f ) {
                                        return 323.0/1432.3;
                                    } else {
                                        return 352.0/1293.1;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals <= 112.5f ) {
                                if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                                    if ( cl->stats.num_overlap_literals <= 20.5f ) {
                                        return 427.0/1779.3;
                                    } else {
                                        return 402.0/1337.5;
                                    }
                                } else {
                                    if ( cl->size() <= 16.5f ) {
                                        return 282.0/1753.1;
                                    } else {
                                        return 357.0/1571.5;
                                    }
                                }
                            } else {
                                return 450.0/1212.4;
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 1653.0f ) {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 10.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 22.5f ) {
                                    return 171.0/2562.0;
                                } else {
                                    return 281.0/2755.7;
                                }
                            } else {
                                return 92.0/2295.7;
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.469880908728f ) {
                                if ( cl->stats.dump_number <= 2.5f ) {
                                    return 220.0/1777.3;
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.407577931881f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.240782678127f ) {
                                            return 199.0/2067.8;
                                        } else {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.0753975957632f ) {
                                                return 189.0/2112.1;
                                            } else {
                                                return 176.0/2935.2;
                                            }
                                        }
                                    } else {
                                        return 245.0/2311.9;
                                    }
                                }
                            } else {
                                if ( cl->size() <= 15.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 3124.0f ) {
                                        if ( cl->size() <= 8.5f ) {
                                            return 192.0/2430.9;
                                        } else {
                                            return 233.0/2261.4;
                                        }
                                    } else {
                                        return 385.0/3042.1;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 7.5f ) {
                                        return 569.0/2917.0;
                                    } else {
                                        return 237.0/2108.1;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_overlap_literals <= 24.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 62284.5f ) {
                            if ( rdb0_last_touched_diff <= 2851.0f ) {
                                if ( rdb0_last_touched_diff <= 749.5f ) {
                                    return 197.0/1722.8;
                                } else {
                                    return 543.0/2975.5;
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 24.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.380083680153f ) {
                                        return 349.0/1400.0;
                                    } else {
                                        return 509.0/2560.0;
                                    }
                                } else {
                                    return 767.0/2384.5;
                                }
                            }
                        } else {
                            if ( cl->size() <= 8.5f ) {
                                return 425.0/1482.7;
                            } else {
                                return 857.0/2251.3;
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_long <= 0.994086325169f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 34057.0f ) {
                                return 515.0/1672.4;
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 105.5f ) {
                                    return 476.0/1289.1;
                                } else {
                                    return 514.0/1024.8;
                                }
                            }
                        } else {
                            return 673.0/1246.7;
                        }
                    }
                }
            } else {
                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                    if ( cl->stats.glue_rel_long <= 0.475158721209f ) {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.230285823345f ) {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 4.5f ) {
                                if ( rdb0_last_touched_diff <= 3395.0f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.0374512597919f ) {
                                        return 218.0/2366.3;
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 8214.5f ) {
                                            if ( rdb0_last_touched_diff <= 777.0f ) {
                                                return 102.0/3498.0;
                                            } else {
                                                if ( cl->stats.used_for_uip_creation <= 9.5f ) {
                                                    return 99.0/2590.2;
                                                } else {
                                                    return 113.0/2057.7;
                                                }
                                            }
                                        } else {
                                            if ( rdb0_last_touched_diff <= 1747.5f ) {
                                                if ( cl->stats.num_antecedents_rel <= 0.124538809061f ) {
                                                    return 130.0/2015.3;
                                                } else {
                                                    return 124.0/3332.6;
                                                }
                                            } else {
                                                return 196.0/1940.7;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 7.5f ) {
                                        return 385.0/2850.5;
                                    } else {
                                        return 233.0/2247.3;
                                    }
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.066689491272f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 9.5f ) {
                                        if ( cl->stats.used_for_uip_creation <= 11.5f ) {
                                            return 164.0/2388.5;
                                        } else {
                                            return 93.0/2382.5;
                                        }
                                    } else {
                                        if ( cl->stats.used_for_uip_creation <= 7.5f ) {
                                            return 100.0/1874.1;
                                        } else {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.00533753726631f ) {
                                                return 22.0/2269.5;
                                            } else {
                                                if ( cl->stats.rdb1_used_for_uip_creation <= 91.5f ) {
                                                    if ( cl->stats.used_for_uip_creation <= 14.5f ) {
                                                        return 94.0/2636.6;
                                                    } else {
                                                        if ( cl->stats.num_total_lits_antecedents <= 21.5f ) {
                                                            if ( cl->stats.size_rel <= 0.0584500394762f ) {
                                                                return 27.0/2257.4;
                                                            } else {
                                                                if ( cl->stats.num_antecedents_rel <= 0.0563278198242f ) {
                                                                    return 80.0/2019.3;
                                                                } else {
                                                                    return 56.0/2846.4;
                                                                }
                                                            }
                                                        } else {
                                                            return 32.0/2582.2;
                                                        }
                                                    }
                                                } else {
                                                    return 111.0/2610.4;
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 10.5f ) {
                                        if ( rdb0_last_touched_diff <= 3163.5f ) {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.120603702962f ) {
                                                return 79.0/2596.3;
                                            } else {
                                                return 49.0/2069.8;
                                            }
                                        } else {
                                            return 98.0/2067.8;
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 2364.5f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 1382.5f ) {
                                                if ( cl->stats.glue <= 5.5f ) {
                                                    if ( cl->stats.num_overlap_literals_rel <= 0.0605905093253f ) {
                                                        return 24.0/2118.2;
                                                    } else {
                                                        return 9.0/3179.3;
                                                    }
                                                } else {
                                                    return 33.0/2880.7;
                                                }
                                            } else {
                                                return 69.0/3798.6;
                                            }
                                        } else {
                                            return 96.0/2735.5;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 9.5f ) {
                                if ( rdb0_last_touched_diff <= 1906.0f ) {
                                    return 183.0/2447.0;
                                } else {
                                    return 334.0/2428.9;
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.247875928879f ) {
                                    if ( cl->stats.size_rel <= 0.210870504379f ) {
                                        return 142.0/3685.6;
                                    } else {
                                        return 183.0/3623.1;
                                    }
                                } else {
                                    return 66.0/2039.5;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 15.5f ) {
                            if ( cl->stats.dump_number <= 8.5f ) {
                                if ( cl->stats.dump_number <= 3.5f ) {
                                    return 91.0/2324.0;
                                } else {
                                    return 69.0/2400.6;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0231811534613f ) {
                                    return 148.0/1793.4;
                                } else {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 6.5f ) {
                                        return 142.0/2412.7;
                                    } else {
                                        return 55.0/2146.4;
                                    }
                                }
                            }
                        } else {
                            if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 3723.0f ) {
                                    if ( rdb0_last_touched_diff <= 4482.0f ) {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 10.5f ) {
                                            return 138.0/2332.0;
                                        } else {
                                            if ( rdb0_last_touched_diff <= 1777.5f ) {
                                                if ( cl->stats.rdb1_last_touched_diff <= 711.5f ) {
                                                    if ( cl->stats.num_antecedents_rel <= 0.199755221605f ) {
                                                        return 59.0/2370.4;
                                                    } else {
                                                        return 39.0/2332.0;
                                                    }
                                                } else {
                                                    return 59.0/3734.1;
                                                }
                                            } else {
                                                return 92.0/2759.7;
                                            }
                                        }
                                    } else {
                                        return 179.0/2211.0;
                                    }
                                } else {
                                    return 166.0/2632.6;
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 2445.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.0938211530447f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 7448.5f ) {
                                            if ( cl->stats.num_antecedents_rel <= 0.0525048077106f ) {
                                                return 95.0/2231.2;
                                            } else {
                                                return 129.0/2154.5;
                                            }
                                        } else {
                                            return 167.0/1797.4;
                                        }
                                    } else {
                                        if ( cl->stats.glue <= 22.5f ) {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                                                if ( cl->stats.used_for_uip_creation <= 10.5f ) {
                                                    if ( cl->size() <= 14.5f ) {
                                                        return 231.0/3653.4;
                                                    } else {
                                                        return 290.0/2965.5;
                                                    }
                                                } else {
                                                    if ( cl->stats.glue_rel_queue <= 0.663972496986f ) {
                                                        return 68.0/2326.0;
                                                    } else {
                                                        return 101.0/2352.2;
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.rdb1_used_for_uip_creation <= 9.5f ) {
                                                    if ( cl->stats.used_for_uip_creation <= 13.5f ) {
                                                        return 88.0/2106.1;
                                                    } else {
                                                        return 50.0/2221.1;
                                                    }
                                                } else {
                                                    if ( cl->stats.num_overlap_literals <= 14.5f ) {
                                                        return 22.0/2219.1;
                                                    } else {
                                                        return 51.0/2816.2;
                                                    }
                                                }
                                            }
                                        } else {
                                            return 96.0/3084.5;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 5.5f ) {
                                        if ( rdb0_last_touched_diff <= 4629.0f ) {
                                            return 272.0/1652.2;
                                        } else {
                                            return 377.0/1753.1;
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 6404.0f ) {
                                            if ( cl->stats.num_antecedents_rel <= 0.1253503263f ) {
                                                return 251.0/2164.6;
                                            } else {
                                                if ( cl->stats.num_total_lits_antecedents <= 74.5f ) {
                                                    return 315.0/3423.4;
                                                } else {
                                                    return 134.0/1757.1;
                                                }
                                            }
                                        } else {
                                            return 249.0/1611.8;
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.dump_number <= 1.5f ) {
                        return 266.0/2251.3;
                    } else {
                        if ( cl->stats.glue_rel_long <= 0.512787818909f ) {
                            if ( cl->size() <= 8.5f ) {
                                return 152.0/2721.4;
                            } else {
                                return 153.0/1983.0;
                            }
                        } else {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 5.5f ) {
                                return 266.0/1718.8;
                            } else {
                                return 109.0/2150.5;
                            }
                        }
                    }
                }
            }
        }
    } else {
        if ( cl->stats.num_overlap_literals <= 31.5f ) {
            if ( rdb0_last_touched_diff <= 95105.5f ) {
                if ( cl->size() <= 11.5f ) {
                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 18.5f ) {
                            if ( cl->stats.num_overlap_literals_rel <= 0.0366576686502f ) {
                                return 644.0/2087.9;
                            } else {
                                return 340.0/1579.6;
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals_rel <= 0.0907035544515f ) {
                                return 541.0/1454.5;
                            } else {
                                return 502.0/1226.5;
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                            if ( cl->stats.num_overlap_literals <= 4.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 41184.0f ) {
                                    if ( rdb0_last_touched_diff <= 35799.5f ) {
                                        return 575.0/1743.0;
                                    } else {
                                        return 666.0/1797.4;
                                    }
                                } else {
                                    return 976.0/1787.3;
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 18.5f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0597620755434f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.408344507217f ) {
                                            return 558.0/966.3;
                                        } else {
                                            return 484.0/1075.2;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.483714967966f ) {
                                            return 410.0/1236.6;
                                        } else {
                                            return 495.0/1321.3;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 0.464679867029f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 38944.0f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.164879053831f ) {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.131937876344f ) {
                                                    return 690.0/1543.3;
                                                } else {
                                                    return 556.0/1065.1;
                                                }
                                            } else {
                                                return 428.0/1293.1;
                                            }
                                        } else {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.118316054344f ) {
                                                return 939.0/1291.1;
                                            } else {
                                                return 624.0/1135.8;
                                            }
                                        }
                                    } else {
                                        return 1039.0/1678.4;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.66252976656f ) {
                                if ( cl->stats.glue_rel_long <= 0.500678300858f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0667831748724f ) {
                                        return 837.0/1373.8;
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 48924.0f ) {
                                            return 534.0/1416.2;
                                        } else {
                                            return 727.0/1279.0;
                                        }
                                    }
                                } else {
                                    return 936.0/1450.5;
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 22.5f ) {
                                    return 773.0/1045.0;
                                } else {
                                    return 818.0/960.2;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_last_touched_diff <= 42419.0f ) {
                        if ( cl->stats.dump_number <= 4.5f ) {
                            if ( cl->stats.size_rel <= 0.870531439781f ) {
                                if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                                    return 975.0/1484.7;
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.0694444477558f ) {
                                        return 693.0/893.7;
                                    } else {
                                        return 1082.0/786.8;
                                    }
                                }
                            } else {
                                return 1622.0/859.4;
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.980000019073f ) {
                                if ( rdb0_last_touched_diff <= 40473.5f ) {
                                    if ( rdb0_last_touched_diff <= 31056.0f ) {
                                        return 568.0/1111.5;
                                    } else {
                                        return 978.0/2126.3;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.625736355782f ) {
                                        return 597.0/1073.2;
                                    } else {
                                        return 792.0/1069.2;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.653357625008f ) {
                                    return 712.0/1162.0;
                                } else {
                                    return 1276.0/1462.6;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.dump_number <= 9.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 57805.5f ) {
                                if ( cl->stats.num_overlap_literals <= 9.5f ) {
                                    return 716.0/734.3;
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.273725032806f ) {
                                        return 880.0/651.6;
                                    } else {
                                        return 942.0/512.4;
                                    }
                                }
                            } else {
                                if ( cl->size() <= 15.5f ) {
                                    return 863.0/542.7;
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.868240237236f ) {
                                        return 1241.0/687.9;
                                    } else {
                                        return 1294.0/298.6;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.471837252378f ) {
                                return 715.0/1018.7;
                            } else {
                                if ( cl->size() <= 14.5f ) {
                                    return 756.0/968.3;
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 13.5f ) {
                                        return 1084.0/1202.3;
                                    } else {
                                        return 1092.0/978.4;
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.antecedents_glue_long_reds_var <= 0.440972208977f ) {
                    if ( cl->stats.num_total_lits_antecedents <= 24.5f ) {
                        if ( rdb0_last_touched_diff <= 267243.0f ) {
                            if ( cl->stats.size_rel <= 0.497381240129f ) {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 6.5f ) {
                                    if ( cl->stats.glue <= 3.5f ) {
                                        return 736.0/940.1;
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0639728456736f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.022036653012f ) {
                                                return 644.0/792.8;
                                            } else {
                                                return 1147.0/1047.0;
                                            }
                                        } else {
                                            return 730.0/1024.8;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.402955174446f ) {
                                        return 1032.0/1125.7;
                                    } else {
                                        return 1372.0/1061.1;
                                    }
                                }
                            } else {
                                return 1140.0/746.4;
                            }
                        } else {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.102047115564f ) {
                                if ( cl->stats.glue_rel_long <= 0.268496513367f ) {
                                    return 792.0/451.9;
                                } else {
                                    return 1027.0/752.5;
                                }
                            } else {
                                return 1287.0/687.9;
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 157789.5f ) {
                            if ( cl->stats.num_antecedents_rel <= 0.312691926956f ) {
                                if ( cl->stats.size_rel <= 0.269544899464f ) {
                                    return 694.0/768.6;
                                } else {
                                    if ( cl->size() <= 18.5f ) {
                                        return 749.0/627.4;
                                    } else {
                                        return 1244.0/889.6;
                                    }
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.344637960196f ) {
                                    return 783.0/500.3;
                                } else {
                                    return 803.0/492.2;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 303904.0f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.352021098137f ) {
                                    if ( cl->size() <= 12.5f ) {
                                        return 1242.0/815.0;
                                    } else {
                                        if ( cl->stats.dump_number <= 22.5f ) {
                                            return 902.0/284.4;
                                        } else {
                                            return 1537.0/811.0;
                                        }
                                    }
                                } else {
                                    return 1206.0/351.0;
                                }
                            } else {
                                if ( cl->stats.dump_number <= 87.5f ) {
                                    if ( cl->stats.dump_number <= 51.5f ) {
                                        return 1657.0/486.2;
                                    } else {
                                        return 1480.0/302.6;
                                    }
                                } else {
                                    return 858.0/441.8;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_act_ranking_top_10 <= 5.5f ) {
                        if ( cl->stats.glue_rel_queue <= 0.78571844101f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 148104.0f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 34.5f ) {
                                    return 713.0/752.5;
                                } else {
                                    return 908.0/702.0;
                                }
                            } else {
                                return 1427.0/702.0;
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 128165.5f ) {
                                return 973.0/530.6;
                            } else {
                                if ( cl->stats.dump_number <= 23.5f ) {
                                    return 940.0/228.0;
                                } else {
                                    return 917.0/266.3;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.dump_number <= 67.5f ) {
                            if ( cl->size() <= 11.5f ) {
                                return 1244.0/712.1;
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.832062125206f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 182215.0f ) {
                                        return 1458.0/562.8;
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.670369684696f ) {
                                            return 912.0/252.2;
                                        } else {
                                            return 967.0/219.9;
                                        }
                                    }
                                } else {
                                    if ( cl->size() <= 18.5f ) {
                                        return 1178.0/332.9;
                                    } else {
                                        if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                            return 1398.0/314.7;
                                        } else {
                                            return 1107.0/106.9;
                                        }
                                    }
                                }
                            }
                        } else {
                            return 1089.0/613.3;
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.glue_rel_queue <= 0.926754117012f ) {
                if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                        if ( cl->stats.size_rel <= 0.485934555531f ) {
                            return 785.0/1619.9;
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.677273988724f ) {
                                return 637.0/772.6;
                            } else {
                                return 1170.0/1153.9;
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 34841.5f ) {
                            if ( cl->stats.dump_number <= 4.5f ) {
                                return 1122.0/1192.2;
                            } else {
                                return 801.0/1458.5;
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 10.3366661072f ) {
                                if ( cl->stats.glue <= 6.5f ) {
                                    return 911.0/1022.8;
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 100.5f ) {
                                        return 898.0/786.8;
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.671300411224f ) {
                                            return 863.0/395.4;
                                        } else {
                                            return 861.0/524.5;
                                        }
                                    }
                                }
                            } else {
                                return 1087.0/466.0;
                            }
                        }
                    }
                } else {
                    if ( cl->size() <= 13.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 92746.5f ) {
                            return 1326.0/1498.9;
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 4.09372806549f ) {
                                if ( cl->stats.glue_rel_long <= 0.5845297575f ) {
                                    return 749.0/585.0;
                                } else {
                                    return 830.0/421.6;
                                }
                            } else {
                                return 873.0/411.5;
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 111499.0f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 228.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 42595.0f ) {
                                    return 1233.0/1101.5;
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.372411608696f ) {
                                        return 988.0/619.3;
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.814022183418f ) {
                                            return 1133.0/756.5;
                                        } else {
                                            return 877.0/296.5;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.736919701099f ) {
                                    return 1021.0/425.7;
                                } else {
                                    return 1381.0/300.6;
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.721598863602f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.216123044491f ) {
                                    return 1064.0/312.7;
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.469502389431f ) {
                                        return 909.0/466.0;
                                    } else {
                                        if ( rdb0_last_touched_diff <= 258993.5f ) {
                                            return 1431.0/574.9;
                                        } else {
                                            return 919.0/234.0;
                                        }
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 195588.0f ) {
                                    if ( cl->stats.dump_number <= 16.5f ) {
                                        return 1226.0/316.7;
                                    } else {
                                        return 944.0/363.1;
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 114.5f ) {
                                        return 1517.0/274.4;
                                    } else {
                                        return 1831.0/425.7;
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                    if ( cl->stats.dump_number <= 13.5f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 202.5f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 80.5f ) {
                                return 946.0/617.3;
                            } else {
                                if ( cl->stats.glue_rel_queue <= 1.20714139938f ) {
                                    if ( rdb0_last_touched_diff <= 59864.0f ) {
                                        return 1535.0/851.3;
                                    } else {
                                        return 1053.0/268.3;
                                    }
                                } else {
                                    return 1220.0/282.4;
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 1.01399111748f ) {
                                return 1228.0/401.4;
                            } else {
                                if ( rdb0_last_touched_diff <= 50576.5f ) {
                                    if ( cl->size() <= 77.5f ) {
                                        return 1195.0/367.2;
                                    } else {
                                        return 990.0/106.9;
                                    }
                                } else {
                                    if ( cl->size() <= 90.5f ) {
                                        return 1611.0/203.7;
                                    } else {
                                        return 1022.0/68.6;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                            return 1000.0/1256.8;
                        } else {
                            return 1040.0/407.5;
                        }
                    }
                } else {
                    if ( cl->size() <= 17.5f ) {
                        if ( cl->stats.glue <= 10.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 67943.0f ) {
                                return 820.0/486.2;
                            } else {
                                if ( cl->stats.num_overlap_literals <= 72.5f ) {
                                    return 874.0/306.6;
                                } else {
                                    return 871.0/353.0;
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 1.12965345383f ) {
                                if ( cl->stats.glue_rel_queue <= 1.03235316277f ) {
                                    return 1123.0/175.5;
                                } else {
                                    return 957.0/278.4;
                                }
                            } else {
                                return 1880.0/296.5;
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_queue <= 1.12048316002f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.357465267181f ) {
                                return 865.0/421.6;
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 609.5f ) {
                                    if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                        if ( cl->stats.rdb1_act_ranking_top_10 <= 4.5f ) {
                                            if ( cl->stats.dump_number <= 16.5f ) {
                                                if ( cl->stats.rdb1_last_touched_diff <= 50735.0f ) {
                                                    return 921.0/217.9;
                                                } else {
                                                    return 973.0/181.6;
                                                }
                                            } else {
                                                return 934.0/306.6;
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 0.966672837734f ) {
                                                return 1003.0/135.2;
                                            } else {
                                                if ( cl->stats.num_overlap_literals <= 64.5f ) {
                                                    return 1293.0/342.9;
                                                } else {
                                                    if ( rdb0_last_touched_diff <= 145760.0f ) {
                                                        return 1834.0/433.7;
                                                    } else {
                                                        return 1505.0/205.8;
                                                    }
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->size() <= 33.5f ) {
                                            return 1046.0/169.5;
                                        } else {
                                            return 1213.0/163.4;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 107268.5f ) {
                                        return 1340.0/163.4;
                                    } else {
                                        return 1627.0/141.2;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.7571362257f ) {
                                return 1540.0/522.5;
                            } else {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 8.5f ) {
                                    if ( cl->stats.glue <= 11.5f ) {
                                        return 1204.0/278.4;
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 22.24817276f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 50758.0f ) {
                                                return 1344.0/304.6;
                                            } else {
                                                if ( cl->stats.num_overlap_literals <= 61.5f ) {
                                                    return 1025.0/179.5;
                                                } else {
                                                    if ( cl->stats.num_overlap_literals <= 322.5f ) {
                                                        if ( cl->size() <= 27.5f ) {
                                                            return 976.0/149.3;
                                                        } else {
                                                            if ( rdb0_last_touched_diff <= 156271.0f ) {
                                                                return 1063.0/131.1;
                                                            } else {
                                                                return 1148.0/72.6;
                                                            }
                                                        }
                                                    } else {
                                                        return 1322.0/78.7;
                                                    }
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 687.5f ) {
                                                if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                                    if ( cl->stats.antecedents_glue_long_reds_var <= 59.9725189209f ) {
                                                        if ( cl->stats.glue_rel_queue <= 1.31218206882f ) {
                                                            return 1059.0/56.5;
                                                        } else {
                                                            return 1153.0/106.9;
                                                        }
                                                    } else {
                                                        return 1735.0/179.5;
                                                    }
                                                } else {
                                                    if ( rdb0_act_ranking_top_10 <= 7.5f ) {
                                                        return 1061.0/159.4;
                                                    } else {
                                                        return 1395.0/143.2;
                                                    }
                                                }
                                            } else {
                                                if ( cl->size() <= 138.5f ) {
                                                    return 962.0/100.9;
                                                } else {
                                                    return 1383.0/58.5;
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 10.4498615265f ) {
                                        return 1488.0/149.3;
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 544.5f ) {
                                            if ( cl->stats.glue_rel_long <= 1.38855457306f ) {
                                                return 1159.0/115.0;
                                            } else {
                                                return 1448.0/44.4;
                                            }
                                        } else {
                                            return 1122.0/26.2;
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

static double estimator_should_keep_short_conf3_cluster0_1(
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
        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
            if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                if ( cl->stats.num_overlap_literals <= 32.5f ) {
                    if ( cl->stats.glue <= 7.5f ) {
                        if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                            if ( rdb0_last_touched_diff <= 10117.5f ) {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 46639.0f ) {
                                        if ( cl->size() <= 9.5f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 24159.5f ) {
                                                if ( cl->stats.glue_rel_long <= 0.411806046963f ) {
                                                    return 229.0/1666.3;
                                                } else {
                                                    return 286.0/1660.3;
                                                }
                                            } else {
                                                return 313.0/1589.6;
                                            }
                                        } else {
                                            return 393.0/1597.7;
                                        }
                                    } else {
                                        return 655.0/1851.9;
                                    }
                                } else {
                                    return 478.0/3078.4;
                                }
                            } else {
                                if ( cl->size() <= 8.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.367872595787f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 15088.5f ) {
                                            return 527.0/2023.4;
                                        } else {
                                            return 731.0/2047.6;
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0809897631407f ) {
                                            if ( rdb0_last_touched_diff <= 26957.0f ) {
                                                return 527.0/2741.5;
                                            } else {
                                                return 469.0/1880.1;
                                            }
                                        } else {
                                            return 584.0/2021.4;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.273748040199f ) {
                                        if ( cl->size() <= 10.5f ) {
                                            return 663.0/2100.0;
                                        } else {
                                            if ( cl->stats.num_overlap_literals <= 8.5f ) {
                                                return 559.0/1470.6;
                                            } else {
                                                return 850.0/1775.2;
                                            }
                                        }
                                    } else {
                                        return 554.0/1202.3;
                                    }
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 2852.5f ) {
                                if ( cl->stats.glue_rel_queue <= 0.732677698135f ) {
                                    if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                        if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                            return 268.0/2842.4;
                                        } else {
                                            return 325.0/2537.8;
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0247251167893f ) {
                                            return 256.0/3251.9;
                                        } else {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.231111109257f ) {
                                                if ( cl->stats.num_total_lits_antecedents <= 11.5f ) {
                                                    return 64.0/2003.2;
                                                } else {
                                                    if ( cl->stats.rdb1_last_touched_diff <= 10582.5f ) {
                                                        return 125.0/1882.2;
                                                    } else {
                                                        if ( cl->stats.rdb1_last_touched_diff <= 21293.0f ) {
                                                            return 69.0/1981.0;
                                                        } else {
                                                            return 140.0/2184.8;
                                                        }
                                                    }
                                                }
                                            } else {
                                                return 151.0/2085.9;
                                            }
                                        }
                                    }
                                } else {
                                    return 178.0/1843.8;
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 7714.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 20243.0f ) {
                                        if ( cl->size() <= 7.5f ) {
                                            if ( cl->stats.num_antecedents_rel <= 0.119068011642f ) {
                                                return 247.0/2033.5;
                                            } else {
                                                return 201.0/2374.4;
                                            }
                                        } else {
                                            return 351.0/2751.6;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 45340.5f ) {
                                            return 408.0/2344.1;
                                        } else {
                                            return 321.0/1379.8;
                                        }
                                    }
                                } else {
                                    return 347.0/1874.1;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                            if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                if ( cl->stats.num_overlap_literals <= 12.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.176662474871f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0908961370587f ) {
                                            return 497.0/1010.7;
                                        } else {
                                            if ( cl->stats.size_rel <= 0.607228040695f ) {
                                                return 427.0/1274.9;
                                            } else {
                                                return 508.0/1034.9;
                                            }
                                        }
                                    } else {
                                        return 619.0/1153.9;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 41311.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.917814075947f ) {
                                            if ( cl->stats.glue_rel_long <= 0.637879133224f ) {
                                                return 559.0/1008.7;
                                            } else {
                                                return 749.0/1664.3;
                                            }
                                        } else {
                                            return 612.0/855.3;
                                        }
                                    } else {
                                        return 764.0/637.5;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 7.5f ) {
                                    return 291.0/2602.3;
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.768712162971f ) {
                                        return 360.0/2765.7;
                                    } else {
                                        return 297.0/1722.8;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.0546875f ) {
                                    return 417.0/1628.0;
                                } else {
                                    return 630.0/1736.9;
                                }
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 7.5f ) {
                                    return 452.0/3007.8;
                                } else {
                                    return 139.0/2660.8;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.glue <= 8.5f ) {
                        if ( rdb0_last_touched_diff <= 9443.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 33483.5f ) {
                                if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                    return 442.0/2104.1;
                                } else {
                                    return 282.0/3052.2;
                                }
                            } else {
                                return 368.0/1474.7;
                            }
                        } else {
                            if ( cl->stats.num_antecedents_rel <= 0.856951117516f ) {
                                if ( cl->size() <= 12.5f ) {
                                    return 455.0/1295.1;
                                } else {
                                    return 926.0/1634.0;
                                }
                            } else {
                                return 452.0/1305.2;
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                            if ( rdb0_last_touched_diff <= 9959.0f ) {
                                if ( cl->size() <= 45.5f ) {
                                    return 447.0/2703.2;
                                } else {
                                    return 295.0/1466.6;
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 1.00126826763f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 6.98979568481f ) {
                                        if ( cl->stats.size_rel <= 0.739239633083f ) {
                                            return 694.0/1172.1;
                                        } else {
                                            return 876.0/1030.9;
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 43821.5f ) {
                                            return 1255.0/1551.3;
                                        } else {
                                            return 891.0/524.5;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 6.81829023361f ) {
                                        return 871.0/607.2;
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.502488732338f ) {
                                            return 782.0/484.2;
                                        } else {
                                            if ( cl->stats.glue <= 18.5f ) {
                                                return 990.0/379.3;
                                            } else {
                                                return 1033.0/197.7;
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 46752.5f ) {
                                return 554.0/2489.4;
                            } else {
                                return 589.0/1619.9;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                    if ( cl->stats.size_rel <= 0.570599079132f ) {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.0713577121496f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 28.5f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.0803873315454f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                                        return 329.0/1539.2;
                                    } else {
                                        return 470.0/2713.3;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                                        return 261.0/1601.8;
                                    } else {
                                        return 284.0/2388.5;
                                    }
                                }
                            } else {
                                return 442.0/1779.3;
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.758437514305f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 3968.0f ) {
                                    if ( cl->size() <= 10.5f ) {
                                        return 317.0/1775.2;
                                    } else {
                                        return 306.0/1490.8;
                                    }
                                } else {
                                    return 609.0/2434.9;
                                }
                            } else {
                                return 403.0/1347.6;
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                            if ( cl->stats.num_antecedents_rel <= 0.310439884663f ) {
                                return 479.0/1250.7;
                            } else {
                                return 568.0/1012.7;
                            }
                        } else {
                            if ( cl->size() <= 30.5f ) {
                                return 430.0/1684.5;
                            } else {
                                return 382.0/1291.1;
                            }
                        }
                    }
                } else {
                    if ( rdb0_last_touched_diff <= 2338.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 1692.5f ) {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 6.5f ) {
                                if ( cl->stats.used_for_uip_creation <= 6.5f ) {
                                    if ( cl->stats.num_overlap_literals <= 11.5f ) {
                                        return 179.0/2834.3;
                                    } else {
                                        return 190.0/1801.5;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.046748355031f ) {
                                        return 122.0/2975.5;
                                    } else {
                                        return 98.0/3671.5;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 16.5f ) {
                                    if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                        return 117.0/1906.4;
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0729905515909f ) {
                                            if ( cl->stats.num_antecedents_rel <= 0.0453715324402f ) {
                                                return 45.0/2085.9;
                                            } else {
                                                return 89.0/2328.0;
                                            }
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 36.5f ) {
                                                if ( cl->stats.num_total_lits_antecedents <= 17.5f ) {
                                                    return 17.0/2112.1;
                                                } else {
                                                    return 34.0/2100.0;
                                                }
                                            } else {
                                                return 97.0/3687.7;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 11.5f ) {
                                        return 120.0/3887.4;
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.0767160877585f ) {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 119.5f ) {
                                                if ( cl->stats.size_rel <= 0.0876061469316f ) {
                                                    if ( cl->stats.num_overlap_literals_rel <= 0.00569838378578f ) {
                                                        return 32.0/2073.8;
                                                    } else {
                                                        return 17.0/2243.3;
                                                    }
                                                } else {
                                                    if ( cl->stats.num_overlap_literals_rel <= 0.0151308812201f ) {
                                                        return 29.0/2136.3;
                                                    } else {
                                                        return 74.0/2572.1;
                                                    }
                                                }
                                            } else {
                                                return 72.0/1884.2;
                                            }
                                        } else {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.0598636046052f ) {
                                                return 54.0/2285.6;
                                            } else {
                                                if ( rdb0_last_touched_diff <= 1374.5f ) {
                                                    if ( cl->stats.dump_number <= 10.5f ) {
                                                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.226813584566f ) {
                                                            if ( cl->stats.num_antecedents_rel <= 0.17447707057f ) {
                                                                return 25.0/2098.0;
                                                            } else {
                                                                return 16.0/2820.2;
                                                            }
                                                        } else {
                                                            return 42.0/2156.5;
                                                        }
                                                    } else {
                                                        if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                                            return 32.0/3615.0;
                                                        } else {
                                                            return 14.0/3324.5;
                                                        }
                                                    }
                                                } else {
                                                    return 38.0/2069.8;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.192376106977f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0334191396832f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 6.5f ) {
                                        if ( cl->size() <= 7.5f ) {
                                            return 180.0/2836.4;
                                        } else {
                                            return 143.0/1858.0;
                                        }
                                    } else {
                                        if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                            return 153.0/3732.0;
                                        } else {
                                            return 65.0/2983.6;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                        return 164.0/2219.1;
                                    } else {
                                        if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                            if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                                return 129.0/2497.4;
                                            } else {
                                                if ( cl->stats.dump_number <= 10.5f ) {
                                                    return 59.0/1952.8;
                                                } else {
                                                    return 52.0/2275.5;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 0.571966648102f ) {
                                                if ( cl->stats.rdb1_last_touched_diff <= 3531.5f ) {
                                                    return 28.0/2303.8;
                                                } else {
                                                    return 52.0/2449.0;
                                                }
                                            } else {
                                                return 50.0/1981.0;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 32.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.545058429241f ) {
                                        return 89.0/2553.9;
                                    } else {
                                        return 94.0/1868.0;
                                    }
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.719834089279f ) {
                                        return 182.0/1876.1;
                                    } else {
                                        if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                            return 232.0/2808.1;
                                        } else {
                                            return 158.0/3116.8;
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue <= 7.5f ) {
                            if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                if ( cl->size() <= 7.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 4330.0f ) {
                                        if ( cl->stats.glue_rel_long <= 0.438681781292f ) {
                                            if ( rdb0_last_touched_diff <= 5889.0f ) {
                                                return 153.0/1928.6;
                                            } else {
                                                return 115.0/1894.3;
                                            }
                                        } else {
                                            return 207.0/2255.4;
                                        }
                                    } else {
                                        return 293.0/2344.1;
                                    }
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.653333306313f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.113871634007f ) {
                                            return 182.0/1724.8;
                                        } else {
                                            return 298.0/2138.4;
                                        }
                                    } else {
                                        return 243.0/1793.4;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.245539367199f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 13.5f ) {
                                        return 189.0/2223.1;
                                    } else {
                                        return 107.0/2051.6;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 5.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.503426074982f ) {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 8.5f ) {
                                                return 127.0/2509.5;
                                            } else {
                                                return 91.0/2983.6;
                                            }
                                        } else {
                                            return 138.0/2263.4;
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.163151413202f ) {
                                            if ( cl->stats.used_for_uip_creation <= 7.5f ) {
                                                return 245.0/3147.0;
                                            } else {
                                                if ( cl->stats.dump_number <= 13.5f ) {
                                                    return 147.0/2773.8;
                                                } else {
                                                    return 129.0/2098.0;
                                                }
                                            }
                                        } else {
                                            return 119.0/2862.6;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 72.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 38.5f ) {
                                        return 213.0/1924.5;
                                    } else {
                                        return 261.0/1767.2;
                                    }
                                } else {
                                    return 413.0/1985.0;
                                }
                            } else {
                                if ( cl->stats.dump_number <= 2.5f ) {
                                    return 254.0/2116.2;
                                } else {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 12.5f ) {
                                        if ( cl->stats.dump_number <= 14.5f ) {
                                            return 148.0/2079.9;
                                        } else {
                                            return 194.0/2202.9;
                                        }
                                    } else {
                                        return 118.0/3030.0;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                if ( cl->stats.glue <= 9.5f ) {
                    if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                        if ( cl->size() <= 13.5f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 4.67447185516f ) {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 24209.0f ) {
                                        if ( cl->size() <= 9.5f ) {
                                            if ( cl->stats.num_total_lits_antecedents <= 16.5f ) {
                                                if ( cl->stats.size_rel <= 0.200193360448f ) {
                                                    return 413.0/1291.1;
                                                } else {
                                                    return 293.0/1557.4;
                                                }
                                            } else {
                                                if ( cl->stats.size_rel <= 0.26178085804f ) {
                                                    return 427.0/1272.9;
                                                } else {
                                                    return 402.0/1361.7;
                                                }
                                            }
                                        } else {
                                            return 699.0/1650.2;
                                        }
                                    } else {
                                        return 727.0/1375.8;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 50358.0f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.0609701126814f ) {
                                            return 626.0/1147.9;
                                        } else {
                                            if ( rdb0_last_touched_diff <= 35224.5f ) {
                                                if ( cl->stats.num_overlap_literals_rel <= 0.118874132633f ) {
                                                    return 685.0/2366.3;
                                                } else {
                                                    return 485.0/1422.2;
                                                }
                                            } else {
                                                if ( cl->stats.num_overlap_literals_rel <= 0.129581153393f ) {
                                                    if ( cl->stats.glue_rel_long <= 0.446738988161f ) {
                                                        return 468.0/1133.7;
                                                    } else {
                                                        return 556.0/1123.6;
                                                    }
                                                } else {
                                                    return 572.0/1020.8;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.192923545837f ) {
                                            return 1203.0/1565.4;
                                        } else {
                                            return 707.0/891.7;
                                        }
                                    }
                                }
                            } else {
                                return 654.0/917.9;
                            }
                        } else {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                if ( rdb0_last_touched_diff <= 24843.0f ) {
                                    return 810.0/1424.2;
                                } else {
                                    return 734.0/895.7;
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 34806.5f ) {
                                    return 1062.0/1525.1;
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 72.5f ) {
                                        return 982.0/1053.0;
                                    } else {
                                        return 801.0/560.8;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 41.5f ) {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.0766388475895f ) {
                                return 372.0/1355.6;
                            } else {
                                return 396.0/2045.6;
                            }
                        } else {
                            return 414.0/1289.1;
                        }
                    }
                } else {
                    if ( cl->stats.antecedents_glue_long_reds_var <= 2.28952383995f ) {
                        if ( rdb0_last_touched_diff <= 44057.5f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.00796659290791f ) {
                                return 929.0/2283.6;
                            } else {
                                return 1073.0/1315.3;
                            }
                        } else {
                            return 1425.0/982.4;
                        }
                    } else {
                        if ( cl->stats.glue_rel_long <= 0.933267533779f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 49032.0f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.387248635292f ) {
                                    return 650.0/825.1;
                                } else {
                                    return 933.0/885.6;
                                }
                            } else {
                                return 1070.0/651.6;
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals <= 273.5f ) {
                                if ( cl->stats.size_rel <= 1.28996396065f ) {
                                    if ( cl->stats.glue <= 12.5f ) {
                                        return 851.0/687.9;
                                    } else {
                                        if ( cl->size() <= 27.5f ) {
                                            return 941.0/349.0;
                                        } else {
                                            return 974.0/447.8;
                                        }
                                    }
                                } else {
                                    return 1383.0/490.2;
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 41418.5f ) {
                                    return 956.0/211.8;
                                } else {
                                    return 1163.0/111.0;
                                }
                            }
                        }
                    }
                }
            } else {
                if ( rdb0_last_touched_diff <= 10028.5f ) {
                    if ( cl->stats.num_overlap_literals <= 7.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 2950.5f ) {
                            if ( rdb0_last_touched_diff <= 5481.0f ) {
                                return 90.0/2140.4;
                            } else {
                                return 126.0/1736.9;
                            }
                        } else {
                            return 205.0/1785.3;
                        }
                    } else {
                        if ( cl->stats.glue_rel_long <= 0.568413853645f ) {
                            return 278.0/3518.2;
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 1977.5f ) {
                                return 229.0/2017.3;
                            } else {
                                return 267.0/1561.4;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.dump_number <= 1.5f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 51.5f ) {
                            return 412.0/1266.9;
                        } else {
                            return 737.0/752.5;
                        }
                    } else {
                        if ( cl->size() <= 7.5f ) {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.0811209976673f ) {
                                return 307.0/1575.5;
                            } else {
                                return 226.0/1652.2;
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 15784.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.0907050296664f ) {
                                    return 323.0/1987.1;
                                } else {
                                    return 510.0/1783.3;
                                }
                            } else {
                                return 508.0/1559.4;
                            }
                        }
                    }
                }
            }
        }
    } else {
        if ( cl->size() <= 11.5f ) {
            if ( cl->stats.rdb1_last_touched_diff <= 46273.0f ) {
                if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                    if ( cl->stats.glue_rel_queue <= 0.818665385246f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 29305.5f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.404081642628f ) {
                                if ( cl->stats.glue <= 3.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.113142766058f ) {
                                        return 381.0/1293.1;
                                    } else {
                                        return 342.0/1402.0;
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 21.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.378111541271f ) {
                                            return 453.0/1210.4;
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 16798.0f ) {
                                                return 350.0/1456.5;
                                            } else {
                                                return 402.0/1293.1;
                                            }
                                        }
                                    } else {
                                        return 791.0/1874.1;
                                    }
                                }
                            } else {
                                return 724.0/1611.8;
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 29.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.167104035616f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.0998912900686f ) {
                                        return 547.0/1200.3;
                                    } else {
                                        return 539.0/1077.2;
                                    }
                                } else {
                                    return 607.0/1408.1;
                                }
                            } else {
                                return 590.0/1028.8;
                            }
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 1.78538131714f ) {
                            if ( cl->size() <= 8.5f ) {
                                return 528.0/1180.1;
                            } else {
                                return 613.0/932.0;
                            }
                        } else {
                            return 989.0/845.3;
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                        return 448.0/2015.3;
                    } else {
                        if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                            return 182.0/1771.2;
                        } else {
                            return 249.0/1853.9;
                        }
                    }
                }
            } else {
                if ( cl->stats.num_total_lits_antecedents <= 19.5f ) {
                    if ( cl->stats.glue_rel_long <= 0.247432991862f ) {
                        if ( rdb0_last_touched_diff <= 157321.5f ) {
                            return 769.0/1034.9;
                        } else {
                            return 961.0/736.3;
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 133348.0f ) {
                            if ( rdb0_last_touched_diff <= 88766.5f ) {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 4.5f ) {
                                    return 1102.0/2001.2;
                                } else {
                                    return 605.0/974.4;
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.511819124222f ) {
                                    return 829.0/1238.6;
                                } else {
                                    return 794.0/936.0;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 274877.5f ) {
                                if ( cl->stats.glue_rel_long <= 0.403786420822f ) {
                                    return 757.0/944.1;
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                        return 696.0/750.4;
                                    } else {
                                        return 1121.0/895.7;
                                    }
                                }
                            } else {
                                return 1352.0/808.9;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.antecedents_glue_long_reds_var <= 9.16758918762f ) {
                        if ( rdb0_last_touched_diff <= 142723.5f ) {
                            if ( cl->stats.glue_rel_queue <= 0.759685993195f ) {
                                if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                    if ( rdb0_last_touched_diff <= 88259.5f ) {
                                        return 872.0/1216.4;
                                    } else {
                                        return 714.0/827.1;
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 25.5f ) {
                                        return 671.0/802.9;
                                    } else {
                                        return 1339.0/1258.8;
                                    }
                                }
                            } else {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 1.47678172588f ) {
                                    return 1079.0/1049.0;
                                } else {
                                    return 919.0/538.6;
                                }
                            }
                        } else {
                            if ( cl->size() <= 6.5f ) {
                                return 1124.0/1109.5;
                            } else {
                                if ( cl->size() <= 9.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 244885.0f ) {
                                        return 1421.0/982.4;
                                    } else {
                                        return 1237.0/597.1;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 203288.5f ) {
                                        return 993.0/589.1;
                                    } else {
                                        return 1731.0/712.1;
                                    }
                                }
                            }
                        }
                    } else {
                        return 1004.0/282.4;
                    }
                }
            }
        } else {
            if ( cl->stats.antecedents_glue_long_reds_var <= 0.258678287268f ) {
                if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                    if ( cl->stats.glue <= 5.5f ) {
                        return 801.0/1690.5;
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.00233639706858f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 6795.5f ) {
                                    return 451.0/1958.8;
                                } else {
                                    return 964.0/1634.0;
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 57250.0f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.824246764183f ) {
                                        return 791.0/1656.2;
                                    } else {
                                        return 686.0/843.2;
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 137009.5f ) {
                                        return 1119.0/897.7;
                                    } else {
                                        return 947.0/451.9;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.size_rel <= 0.701072692871f ) {
                                return 959.0/1073.2;
                            } else {
                                return 1019.0/572.9;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.dump_number <= 15.5f ) {
                        if ( cl->stats.glue_rel_long <= 0.615588128567f ) {
                            if ( cl->stats.glue_rel_long <= 0.346049785614f ) {
                                return 557.0/1151.9;
                            } else {
                                if ( cl->size() <= 20.5f ) {
                                    return 681.0/677.8;
                                } else {
                                    return 819.0/1228.5;
                                }
                            }
                        } else {
                            if ( cl->stats.size_rel <= 0.77282845974f ) {
                                if ( cl->stats.glue <= 13.5f ) {
                                    return 1489.0/1006.6;
                                } else {
                                    return 956.0/1073.2;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 24.5f ) {
                                    return 1520.0/960.2;
                                } else {
                                    return 1107.0/486.2;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_queue <= 0.892226934433f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.112643167377f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 150060.0f ) {
                                    return 848.0/1099.4;
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.264316111803f ) {
                                        if ( cl->stats.glue_rel_long <= 0.421222925186f ) {
                                            return 808.0/470.0;
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 0.625806987286f ) {
                                                return 930.0/375.2;
                                            } else {
                                                return 852.0/425.7;
                                            }
                                        }
                                    } else {
                                        return 1507.0/512.4;
                                    }
                                }
                            } else {
                                return 1492.0/601.2;
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 183103.0f ) {
                                return 800.0/510.4;
                            } else {
                                if ( cl->stats.size_rel <= 0.759589135647f ) {
                                    return 880.0/286.5;
                                } else {
                                    return 1715.0/345.0;
                                }
                            }
                        }
                    }
                }
            } else {
                if ( rdb0_last_touched_diff <= 46227.5f ) {
                    if ( cl->stats.glue_rel_queue <= 0.881186723709f ) {
                        if ( cl->size() <= 88.5f ) {
                            if ( rdb0_last_touched_diff <= 17301.0f ) {
                                return 622.0/1297.1;
                            } else {
                                if ( cl->size() <= 23.5f ) {
                                    if ( cl->stats.size_rel <= 0.523009419441f ) {
                                        return 711.0/685.9;
                                    } else {
                                        return 993.0/1452.5;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 22475.5f ) {
                                        return 820.0/679.8;
                                    } else {
                                        return 799.0/482.1;
                                    }
                                }
                            }
                        } else {
                            return 1019.0/445.8;
                        }
                    } else {
                        if ( cl->stats.num_overlap_literals_rel <= 0.846587300301f ) {
                            if ( cl->stats.num_overlap_literals <= 99.5f ) {
                                if ( cl->stats.glue <= 10.5f ) {
                                    return 1119.0/960.2;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 11430.0f ) {
                                        return 860.0/560.8;
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 1.09580755234f ) {
                                            return 930.0/447.8;
                                        } else {
                                            return 1395.0/427.7;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->size() <= 86.5f ) {
                                    return 983.0/175.5;
                                } else {
                                    return 1255.0/334.9;
                                }
                            }
                        } else {
                            if ( cl->size() <= 28.5f ) {
                                if ( cl->stats.glue_rel_long <= 1.09577143192f ) {
                                    return 856.0/395.4;
                                } else {
                                    return 1343.0/363.1;
                                }
                            } else {
                                if ( cl->stats.glue <= 27.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 422.5f ) {
                                        return 1165.0/345.0;
                                    } else {
                                        return 965.0/153.3;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 17713.0f ) {
                                        return 965.0/169.5;
                                    } else {
                                        return 1212.0/106.9;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_total_lits_antecedents <= 121.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 111440.0f ) {
                            if ( cl->stats.glue_rel_long <= 0.911013364792f ) {
                                if ( cl->stats.size_rel <= 0.914446949959f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 3.56944441795f ) {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 1.60468745232f ) {
                                            return 1201.0/911.8;
                                        } else {
                                            return 883.0/788.8;
                                        }
                                    } else {
                                        if ( cl->stats.glue <= 10.5f ) {
                                            return 1050.0/702.0;
                                        } else {
                                            return 874.0/441.8;
                                        }
                                    }
                                } else {
                                    return 1621.0/760.5;
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 49216.5f ) {
                                    return 1217.0/488.2;
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.529594242573f ) {
                                        if ( cl->stats.dump_number <= 10.5f ) {
                                            if ( rdb0_last_touched_diff <= 81731.0f ) {
                                                return 1060.0/213.8;
                                            } else {
                                                return 933.0/246.1;
                                            }
                                        } else {
                                            return 895.0/433.7;
                                        }
                                    } else {
                                        return 1534.0/296.5;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.399549484253f ) {
                                if ( cl->stats.dump_number <= 67.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.888793468475f ) {
                                        if ( rdb0_last_touched_diff <= 313262.0f ) {
                                            if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                                return 1114.0/683.9;
                                            } else {
                                                if ( cl->size() <= 19.5f ) {
                                                    return 1700.0/700.0;
                                                } else {
                                                    return 1607.0/445.8;
                                                }
                                            }
                                        } else {
                                            return 1254.0/284.4;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.988188803196f ) {
                                            return 1071.0/282.4;
                                        } else {
                                            if ( rdb0_last_touched_diff <= 196570.5f ) {
                                                return 945.0/167.4;
                                            } else {
                                                return 1150.0/157.4;
                                            }
                                        }
                                    }
                                } else {
                                    return 998.0/496.3;
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.792554080486f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 270712.5f ) {
                                        return 1458.0/617.3;
                                    } else {
                                        return 937.0/217.9;
                                    }
                                } else {
                                    if ( cl->stats.glue <= 12.5f ) {
                                        if ( cl->stats.size_rel <= 0.884647846222f ) {
                                            return 1083.0/373.2;
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 75.5f ) {
                                                return 1285.0/151.3;
                                            } else {
                                                return 1323.0/242.1;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 43.5f ) {
                                            return 1826.0/185.6;
                                        } else {
                                            if ( cl->stats.rdb1_act_ranking_top_10 <= 6.5f ) {
                                                return 994.0/123.1;
                                            } else {
                                                return 1214.0/258.2;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_queue <= 0.908347427845f ) {
                            if ( rdb0_last_touched_diff <= 148078.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.346168994904f ) {
                                    return 1257.0/298.6;
                                } else {
                                    if ( cl->stats.size_rel <= 0.763691663742f ) {
                                        return 1103.0/657.6;
                                    } else {
                                        if ( cl->stats.glue <= 11.5f ) {
                                            return 872.0/413.6;
                                        } else {
                                            return 1018.0/306.6;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.856359362602f ) {
                                    if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                        return 986.0/282.4;
                                    } else {
                                        return 989.0/197.7;
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 1.30171442032f ) {
                                        return 983.0/373.2;
                                    } else {
                                        return 933.0/262.3;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 1.28414082527f ) {
                                if ( cl->stats.num_overlap_literals <= 718.5f ) {
                                    if ( cl->size() <= 116.5f ) {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 42.4611129761f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 2.5979449749f ) {
                                                if ( cl->stats.glue <= 10.5f ) {
                                                    return 1551.0/476.1;
                                                } else {
                                                    if ( cl->stats.glue_rel_queue <= 1.10538685322f ) {
                                                        if ( cl->size() <= 20.5f ) {
                                                            return 1167.0/161.4;
                                                        } else {
                                                            if ( cl->stats.dump_number <= 25.5f ) {
                                                                if ( cl->size() <= 39.5f ) {
                                                                    return 1092.0/189.6;
                                                                } else {
                                                                    return 1213.0/308.7;
                                                                }
                                                            } else {
                                                                return 1020.0/306.6;
                                                            }
                                                        }
                                                    } else {
                                                        if ( cl->stats.dump_number <= 26.5f ) {
                                                            if ( cl->stats.antec_num_total_lits_rel <= 1.04473733902f ) {
                                                                return 955.0/161.4;
                                                            } else {
                                                                return 1710.0/157.4;
                                                            }
                                                        } else {
                                                            return 1014.0/195.7;
                                                        }
                                                    }
                                                }
                                            } else {
                                                return 1464.0/443.8;
                                            }
                                        } else {
                                            if ( cl->stats.dump_number <= 18.5f ) {
                                                if ( cl->stats.antecedents_glue_long_reds_var <= 94.8586425781f ) {
                                                    return 1125.0/58.5;
                                                } else {
                                                    return 973.0/139.2;
                                                }
                                            } else {
                                                return 1045.0/155.3;
                                            }
                                        }
                                    } else {
                                        if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                            return 1500.0/108.9;
                                        } else {
                                            return 1605.0/221.9;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 18.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 3.12756824493f ) {
                                            return 1212.0/46.4;
                                        } else {
                                            return 1137.0/72.6;
                                        }
                                    } else {
                                        return 1011.0/143.2;
                                    }
                                }
                            } else {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 4.18905830383f ) {
                                    return 1276.0/199.7;
                                } else {
                                    if ( cl->stats.glue_rel_long <= 1.35705542564f ) {
                                        if ( cl->stats.num_overlap_literals <= 235.5f ) {
                                            return 1358.0/193.7;
                                        } else {
                                            return 1056.0/86.7;
                                        }
                                    } else {
                                        if ( cl->size() <= 33.5f ) {
                                            return 1722.0/207.8;
                                        } else {
                                            if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                                if ( rdb0_last_touched_diff <= 173465.5f ) {
                                                    if ( cl->size() <= 110.5f ) {
                                                        return 1493.0/145.2;
                                                    } else {
                                                        return 1012.0/54.5;
                                                    }
                                                } else {
                                                    return 1353.0/50.4;
                                                }
                                            } else {
                                                return 1830.0/52.5;
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

static double estimator_should_keep_short_conf3_cluster0_2(
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
        if ( cl->stats.num_overlap_literals_rel <= 0.311060875654f ) {
            if ( cl->stats.antecedents_glue_long_reds_var <= 0.308390021324f ) {
                if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                    if ( cl->stats.glue <= 7.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 23631.0f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 21.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 16897.0f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                        if ( cl->size() <= 8.5f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 13437.5f ) {
                                                if ( cl->stats.glue_rel_queue <= 0.519255876541f ) {
                                                    if ( cl->stats.num_total_lits_antecedents <= 13.5f ) {
                                                        if ( cl->stats.num_overlap_literals_rel <= 0.0277580171824f ) {
                                                            return 381.0/1404.1;
                                                        } else {
                                                            return 313.0/1478.7;
                                                        }
                                                    } else {
                                                        return 471.0/1527.1;
                                                    }
                                                } else {
                                                    return 400.0/1886.2;
                                                }
                                            } else {
                                                return 406.0/1999.2;
                                            }
                                        } else {
                                            return 471.0/1402.0;
                                        }
                                    } else {
                                        if ( cl->stats.dump_number <= 52.5f ) {
                                            if ( cl->stats.size_rel <= 0.196250423789f ) {
                                                if ( cl->stats.dump_number <= 18.5f ) {
                                                    if ( cl->stats.rdb1_last_touched_diff <= 3131.0f ) {
                                                        return 257.0/1555.4;
                                                    } else {
                                                        return 266.0/1486.8;
                                                    }
                                                } else {
                                                    return 344.0/1496.9;
                                                }
                                            } else {
                                                if ( cl->stats.rdb1_last_touched_diff <= 3721.5f ) {
                                                    return 340.0/2723.4;
                                                } else {
                                                    return 440.0/2186.8;
                                                }
                                            }
                                        } else {
                                            return 199.0/1740.9;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0712370052934f ) {
                                        return 437.0/1216.4;
                                    } else {
                                        return 445.0/1539.2;
                                    }
                                }
                            } else {
                                if ( cl->size() <= 10.5f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 10929.0f ) {
                                            return 747.0/2535.8;
                                        } else {
                                            return 628.0/1809.5;
                                        }
                                    } else {
                                        return 290.0/1619.9;
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 15082.5f ) {
                                        return 507.0/1847.9;
                                    } else {
                                        if ( cl->stats.size_rel <= 0.447338521481f ) {
                                            return 497.0/1164.0;
                                        } else {
                                            return 485.0/1373.8;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 18.5f ) {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.36174684763f ) {
                                        return 479.0/1323.4;
                                    } else {
                                        return 478.0/1660.3;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.373496502638f ) {
                                        return 651.0/1079.3;
                                    } else {
                                        return 736.0/1775.2;
                                    }
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                    return 806.0/1805.5;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 40711.0f ) {
                                        return 732.0/1327.4;
                                    } else {
                                        return 1141.0/1234.6;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 44093.5f ) {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 4.5f ) {
                                if ( cl->stats.size_rel <= 0.714393258095f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 37.5f ) {
                                            return 831.0/2303.8;
                                        } else {
                                            return 596.0/1077.2;
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.051059845835f ) {
                                            return 454.0/1180.1;
                                        } else {
                                            return 466.0/1672.4;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.836490988731f ) {
                                        return 504.0/1069.2;
                                    } else {
                                        return 604.0/1026.8;
                                    }
                                }
                            } else {
                                return 488.0/2303.8;
                            }
                        } else {
                            if ( cl->size() <= 24.5f ) {
                                return 1163.0/1404.1;
                            } else {
                                return 747.0/607.2;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                        if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 23.5f ) {
                                if ( cl->stats.size_rel <= 0.219242453575f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 26589.0f ) {
                                            return 497.0/2848.5;
                                        } else {
                                            return 518.0/1767.2;
                                        }
                                    } else {
                                        return 266.0/1954.8;
                                    }
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                                        if ( cl->stats.glue <= 5.5f ) {
                                            return 408.0/2626.6;
                                        } else {
                                            return 372.0/2073.8;
                                        }
                                    } else {
                                        return 403.0/2911.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 32437.5f ) {
                                    if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.15516884625f ) {
                                            return 426.0/1478.7;
                                        } else {
                                            return 431.0/2083.9;
                                        }
                                    } else {
                                        return 434.0/2315.9;
                                    }
                                } else {
                                    return 733.0/1983.0;
                                }
                            }
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 6.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.049213051796f ) {
                                    return 360.0/1730.9;
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 27.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 18775.5f ) {
                                            if ( rdb0_last_touched_diff <= 4233.0f ) {
                                                if ( cl->stats.num_overlap_literals_rel <= 0.0442824289203f ) {
                                                    return 145.0/1761.1;
                                                } else {
                                                    return 157.0/2606.4;
                                                }
                                            } else {
                                                return 217.0/1765.2;
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 35459.5f ) {
                                                return 199.0/1825.7;
                                            } else {
                                                return 233.0/1722.8;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.158467888832f ) {
                                            return 263.0/1575.5;
                                        } else {
                                            return 346.0/2628.6;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0181616656482f ) {
                                    return 314.0/3373.0;
                                } else {
                                    if ( cl->stats.dump_number <= 9.5f ) {
                                        if ( rdb0_last_touched_diff <= 1750.0f ) {
                                            return 139.0/3078.4;
                                        } else {
                                            return 247.0/1995.1;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                                            if ( cl->stats.num_overlap_literals <= 5.5f ) {
                                                return 123.0/2301.8;
                                            } else {
                                                if ( cl->stats.rdb1_last_touched_diff <= 23517.0f ) {
                                                    return 111.0/1864.0;
                                                } else {
                                                    return 170.0/1843.8;
                                                }
                                            }
                                        } else {
                                            return 92.0/2041.5;
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 2338.5f ) {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.069903485477f ) {
                                if ( rdb0_last_touched_diff <= 1454.5f ) {
                                    if ( cl->stats.used_for_uip_creation <= 5.5f ) {
                                        return 282.0/3518.2;
                                    } else {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 9.5f ) {
                                            if ( cl->stats.size_rel <= 0.0725615248084f ) {
                                                return 94.0/1999.2;
                                            } else {
                                                if ( cl->stats.dump_number <= 11.5f ) {
                                                    return 57.0/2106.1;
                                                } else {
                                                    return 89.0/1962.9;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 134.5f ) {
                                                if ( cl->stats.size_rel <= 0.028373286128f ) {
                                                    return 11.0/2307.8;
                                                } else {
                                                    if ( rdb0_last_touched_diff <= 268.5f ) {
                                                        if ( cl->stats.used_for_uip_creation <= 45.5f ) {
                                                            return 74.0/3370.9;
                                                        } else {
                                                            return 25.0/2808.1;
                                                        }
                                                    } else {
                                                        if ( cl->stats.glue_rel_queue <= 0.239607065916f ) {
                                                            return 81.0/1960.8;
                                                        } else {
                                                            if ( cl->stats.glue_rel_queue <= 0.483714967966f ) {
                                                                return 61.0/3518.2;
                                                            } else {
                                                                return 62.0/1926.5;
                                                            }
                                                        }
                                                    }
                                                }
                                            } else {
                                                return 86.0/1898.3;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 7.5f ) {
                                        return 242.0/2309.8;
                                    } else {
                                        return 135.0/3717.9;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 1044.5f ) {
                                    if ( cl->stats.dump_number <= 3.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.473784148693f ) {
                                            return 70.0/2112.1;
                                        } else {
                                            return 44.0/2414.7;
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 749.5f ) {
                                            if ( cl->stats.used_for_uip_creation <= 14.5f ) {
                                                return 66.0/2511.6;
                                            } else {
                                                if ( cl->stats.num_overlap_literals_rel <= 0.0567194446921f ) {
                                                    return 33.0/2693.1;
                                                } else {
                                                    if ( cl->stats.glue_rel_queue <= 0.44130936265f ) {
                                                        return 13.0/2051.6;
                                                    } else {
                                                        return 8.0/2495.4;
                                                    }
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 37.5f ) {
                                                return 91.0/3834.9;
                                            } else {
                                                return 38.0/2025.4;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 1.5f ) {
                                        return 150.0/2198.9;
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 3069.0f ) {
                                            if ( cl->stats.glue_rel_long <= 0.491711705923f ) {
                                                if ( cl->stats.size_rel <= 0.306968450546f ) {
                                                    return 106.0/3473.8;
                                                } else {
                                                    return 39.0/2225.1;
                                                }
                                            } else {
                                                if ( cl->stats.used_for_uip_creation <= 13.5f ) {
                                                    return 125.0/2580.2;
                                                } else {
                                                    return 36.0/2174.7;
                                                }
                                            }
                                        } else {
                                            if ( rdb0_last_touched_diff <= 665.5f ) {
                                                if ( rdb0_last_touched_diff <= 233.0f ) {
                                                    return 49.0/2279.6;
                                                } else {
                                                    return 73.0/2315.9;
                                                }
                                            } else {
                                                if ( cl->stats.used_for_uip_creation <= 5.5f ) {
                                                    return 169.0/2112.1;
                                                } else {
                                                    return 99.0/3032.0;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 5044.5f ) {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 9.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 18.5f ) {
                                        if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                            return 254.0/2943.3;
                                        } else {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0319229289889f ) {
                                                return 148.0/1964.9;
                                            } else {
                                                return 98.0/2473.2;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 1363.0f ) {
                                            return 166.0/2087.9;
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 36.5f ) {
                                                return 275.0/2566.0;
                                            } else {
                                                return 219.0/1753.1;
                                            }
                                        }
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 3666.5f ) {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 19.5f ) {
                                            return 125.0/3026.0;
                                        } else {
                                            return 88.0/3534.3;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.383713245392f ) {
                                            return 142.0/2172.7;
                                        } else {
                                            return 101.0/2396.6;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 14.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.08459430933f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0163671597838f ) {
                                            return 129.0/1997.1;
                                        } else {
                                            return 274.0/3106.7;
                                        }
                                    } else {
                                        return 122.0/2471.2;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 9.5f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.0870796218514f ) {
                                            return 312.0/1882.2;
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 28.5f ) {
                                                return 320.0/3393.1;
                                            } else {
                                                return 310.0/2566.0;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.glue <= 5.5f ) {
                                            return 177.0/2029.4;
                                        } else {
                                            return 207.0/2673.0;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.glue_rel_long <= 0.834753990173f ) {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                        if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 26390.0f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 19038.0f ) {
                                    if ( cl->size() <= 10.5f ) {
                                        return 645.0/2461.1;
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 36.5f ) {
                                            return 679.0/1785.3;
                                        } else {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.114774219692f ) {
                                                return 620.0/1002.6;
                                            } else {
                                                return 775.0/1732.9;
                                            }
                                        }
                                    }
                                } else {
                                    return 870.0/1640.1;
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                    if ( cl->size() <= 16.5f ) {
                                        return 611.0/1341.5;
                                    } else {
                                        return 811.0/1006.6;
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 60696.5f ) {
                                        return 1048.0/1333.4;
                                    } else {
                                        return 1387.0/1226.5;
                                    }
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 3176.5f ) {
                                if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                    return 462.0/2087.9;
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.181304335594f ) {
                                        return 264.0/2999.8;
                                    } else {
                                        return 161.0/2114.2;
                                    }
                                }
                            } else {
                                if ( cl->size() <= 10.5f ) {
                                    return 316.0/1920.5;
                                } else {
                                    if ( rdb0_last_touched_diff <= 5605.5f ) {
                                        return 401.0/1692.5;
                                    } else {
                                        return 575.0/1724.8;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.size_rel <= 0.424636185169f ) {
                            if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 20.5f ) {
                                    return 225.0/2493.4;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 4326.0f ) {
                                        if ( cl->stats.glue <= 5.5f ) {
                                            return 156.0/1837.8;
                                        } else {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 9.5f ) {
                                                return 296.0/1452.5;
                                            } else {
                                                return 136.0/2362.3;
                                            }
                                        }
                                    } else {
                                        return 409.0/1833.7;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0406396761537f ) {
                                    return 73.0/2120.2;
                                } else {
                                    return 174.0/3399.2;
                                }
                            }
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                                if ( cl->stats.glue_rel_long <= 0.614545702934f ) {
                                    return 460.0/1262.8;
                                } else {
                                    return 452.0/1515.0;
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.16645899415f ) {
                                    return 192.0/2065.7;
                                } else {
                                    if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                        if ( rdb0_last_touched_diff <= 3051.0f ) {
                                            return 108.0/1892.2;
                                        } else {
                                            return 213.0/1704.6;
                                        }
                                    } else {
                                        return 102.0/2414.7;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.dump_number <= 13.5f ) {
                        if ( cl->size() <= 11.5f ) {
                            return 384.0/1966.9;
                        } else {
                            if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                    if ( cl->stats.glue_rel_long <= 1.12909793854f ) {
                                        if ( rdb0_last_touched_diff <= 30648.5f ) {
                                            return 1040.0/1363.7;
                                        } else {
                                            return 1319.0/849.3;
                                        }
                                    } else {
                                        return 1067.0/429.7;
                                    }
                                } else {
                                    return 363.0/1938.6;
                                }
                            } else {
                                return 520.0/2814.2;
                            }
                        }
                    } else {
                        if ( cl->size() <= 14.5f ) {
                            return 364.0/1484.7;
                        } else {
                            if ( rdb0_last_touched_diff <= 8016.0f ) {
                                return 367.0/2364.3;
                            } else {
                                return 878.0/1337.5;
                            }
                        }
                    }
                }
            }
        } else {
            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                if ( rdb0_last_touched_diff <= 9985.5f ) {
                    if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                        if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                            if ( cl->stats.glue_rel_long <= 1.03054845333f ) {
                                if ( cl->stats.num_overlap_literals <= 128.5f ) {
                                    if ( rdb0_last_touched_diff <= 4142.5f ) {
                                        return 258.0/2521.7;
                                    } else {
                                        return 335.0/2150.5;
                                    }
                                } else {
                                    return 345.0/2003.2;
                                }
                            } else {
                                return 352.0/1373.8;
                            }
                        } else {
                            if ( cl->size() <= 18.5f ) {
                                return 545.0/2332.0;
                            } else {
                                return 677.0/1870.1;
                            }
                        }
                    } else {
                        if ( cl->stats.glue <= 18.5f ) {
                            if ( cl->stats.size_rel <= 0.42755407095f ) {
                                if ( rdb0_last_touched_diff <= 1040.5f ) {
                                    return 35.0/2624.5;
                                } else {
                                    return 152.0/2582.2;
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 1011.5f ) {
                                    return 100.0/2689.1;
                                } else {
                                    return 334.0/3318.5;
                                }
                            }
                        } else {
                            return 131.0/1855.9;
                        }
                    }
                } else {
                    if ( cl->stats.dump_number <= 11.5f ) {
                        if ( cl->stats.num_overlap_literals <= 70.5f ) {
                            if ( rdb0_last_touched_diff <= 31179.0f ) {
                                if ( cl->stats.size_rel <= 0.749516487122f ) {
                                    return 347.0/1377.8;
                                } else {
                                    return 610.0/831.1;
                                }
                            } else {
                                return 766.0/550.7;
                            }
                        } else {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                if ( cl->stats.glue <= 9.5f ) {
                                    return 789.0/1478.7;
                                } else {
                                    if ( cl->stats.glue_rel_long <= 1.03985285759f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.95249158144f ) {
                                            return 1010.0/956.2;
                                        } else {
                                            return 807.0/435.7;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 16900.0f ) {
                                            return 870.0/373.2;
                                        } else {
                                            return 1156.0/213.8;
                                        }
                                    }
                                }
                            } else {
                                return 555.0/1408.1;
                            }
                        }
                    } else {
                        if ( cl->size() <= 13.5f ) {
                            return 367.0/1389.9;
                        } else {
                            if ( cl->stats.dump_number <= 23.5f ) {
                                return 583.0/1018.7;
                            } else {
                                return 500.0/1206.4;
                            }
                        }
                    }
                }
            } else {
                if ( rdb0_last_touched_diff <= 10011.0f ) {
                    return 586.0/2652.8;
                } else {
                    if ( cl->size() <= 16.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 42606.5f ) {
                            if ( cl->stats.glue_rel_long <= 0.937279164791f ) {
                                if ( cl->stats.glue <= 7.5f ) {
                                    return 706.0/2424.8;
                                } else {
                                    return 623.0/1109.5;
                                }
                            } else {
                                return 859.0/869.5;
                            }
                        } else {
                            return 814.0/702.0;
                        }
                    } else {
                        if ( cl->stats.glue_rel_queue <= 0.953831374645f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                return 1073.0/1335.5;
                            } else {
                                if ( cl->stats.size_rel <= 0.940186619759f ) {
                                    return 764.0/663.7;
                                } else {
                                    return 869.0/621.3;
                                }
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 24.0432167053f ) {
                                if ( cl->stats.num_overlap_literals <= 253.5f ) {
                                    if ( cl->stats.glue <= 12.5f ) {
                                        return 836.0/685.9;
                                    } else {
                                        return 1232.0/651.6;
                                    }
                                } else {
                                    return 959.0/195.7;
                                }
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.721141636372f ) {
                                    return 941.0/258.2;
                                } else {
                                    return 1417.0/217.9;
                                }
                            }
                        }
                    }
                }
            }
        }
    } else {
        if ( cl->stats.size_rel <= 0.650632858276f ) {
            if ( cl->stats.glue_rel_long <= 0.835454583168f ) {
                if ( rdb0_last_touched_diff <= 64510.5f ) {
                    if ( cl->stats.rdb1_last_touched_diff <= 19923.0f ) {
                        if ( rdb0_last_touched_diff <= 10687.0f ) {
                            if ( cl->stats.glue_rel_long <= 0.460695236921f ) {
                                return 275.0/2128.3;
                            } else {
                                return 377.0/1868.0;
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 63.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.0615430288017f ) {
                                    if ( cl->stats.num_overlap_literals <= 6.5f ) {
                                        return 428.0/1369.8;
                                    } else {
                                        return 557.0/936.0;
                                    }
                                } else {
                                    if ( cl->size() <= 8.5f ) {
                                        if ( cl->stats.num_overlap_literals <= 7.5f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0477048009634f ) {
                                                return 395.0/1426.2;
                                            } else {
                                                return 371.0/1613.9;
                                            }
                                        } else {
                                            return 457.0/2053.6;
                                        }
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.0994189381599f ) {
                                            return 701.0/2243.3;
                                        } else {
                                            return 590.0/1460.5;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.484998524189f ) {
                                    return 500.0/1153.9;
                                } else {
                                    return 1059.0/1248.7;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_queue <= 0.605366110802f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.404861092567f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.0380836054683f ) {
                                    return 712.0/1129.7;
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 40.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0341532155871f ) {
                                            return 680.0/1359.7;
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.232746362686f ) {
                                                if ( cl->stats.dump_number <= 6.5f ) {
                                                    return 506.0/1121.6;
                                                } else {
                                                    return 710.0/1920.5;
                                                }
                                            } else {
                                                return 654.0/1317.3;
                                            }
                                        }
                                    } else {
                                        return 756.0/1468.6;
                                    }
                                }
                            } else {
                                return 1235.0/1623.9;
                            }
                        } else {
                            if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                if ( cl->stats.num_overlap_literals <= 21.5f ) {
                                    return 1051.0/1936.6;
                                } else {
                                    return 885.0/1026.8;
                                }
                            } else {
                                return 1368.0/1246.7;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_total_lits_antecedents <= 23.5f ) {
                        if ( cl->stats.dump_number <= 15.5f ) {
                            if ( cl->stats.num_overlap_literals <= 6.5f ) {
                                if ( cl->stats.glue_rel_queue <= 0.490645945072f ) {
                                    return 918.0/1250.7;
                                } else {
                                    return 764.0/724.2;
                                }
                            } else {
                                return 845.0/1174.1;
                            }
                        } else {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 4.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 153938.0f ) {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                                        return 1022.0/1521.1;
                                    } else {
                                        return 686.0/879.6;
                                    }
                                } else {
                                    return 836.0/681.9;
                                }
                            } else {
                                if ( cl->stats.dump_number <= 23.5f ) {
                                    return 1317.0/1248.7;
                                } else {
                                    if ( cl->stats.size_rel <= 0.442824751139f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 264476.0f ) {
                                            if ( rdb0_last_touched_diff <= 171341.5f ) {
                                                return 761.0/821.1;
                                            } else {
                                                return 1126.0/1008.7;
                                            }
                                        } else {
                                            if ( cl->stats.size_rel <= 0.16148366034f ) {
                                                return 903.0/685.9;
                                            } else {
                                                return 954.0/510.4;
                                            }
                                        }
                                    } else {
                                        return 854.0/462.0;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue <= 5.5f ) {
                            if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.208105906844f ) {
                                    return 1100.0/1043.0;
                                } else {
                                    return 616.0/837.2;
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.337420105934f ) {
                                    return 766.0/643.5;
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.50429880619f ) {
                                        return 850.0/447.8;
                                    } else {
                                        return 863.0/530.6;
                                    }
                                }
                            }
                        } else {
                            if ( cl->size() <= 11.5f ) {
                                if ( cl->size() <= 8.5f ) {
                                    return 1152.0/976.4;
                                } else {
                                    if ( cl->stats.size_rel <= 0.449455618858f ) {
                                        if ( cl->stats.dump_number <= 22.5f ) {
                                            return 833.0/566.9;
                                        } else {
                                            return 971.0/506.3;
                                        }
                                    } else {
                                        return 847.0/802.9;
                                    }
                                }
                            } else {
                                if ( cl->stats.dump_number <= 18.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 113372.5f ) {
                                        if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                            return 1506.0/911.8;
                                        } else {
                                            return 987.0/907.8;
                                        }
                                    } else {
                                        return 1613.0/710.1;
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 129803.0f ) {
                                        return 722.0/667.7;
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 312033.5f ) {
                                            if ( cl->stats.glue_rel_queue <= 0.726266682148f ) {
                                                if ( cl->stats.glue <= 9.5f ) {
                                                    return 1310.0/603.2;
                                                } else {
                                                    return 1120.0/627.4;
                                                }
                                            } else {
                                                return 910.0/336.9;
                                            }
                                        } else {
                                            if ( cl->stats.dump_number <= 68.5f ) {
                                                return 1428.0/345.0;
                                            } else {
                                                return 879.0/387.3;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.glue <= 9.5f ) {
                    if ( cl->stats.dump_number <= 6.5f ) {
                        if ( cl->stats.num_overlap_literals <= 21.5f ) {
                            return 757.0/1577.5;
                        } else {
                            return 1213.0/1204.3;
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 55853.5f ) {
                            return 528.0/1115.6;
                        } else {
                            if ( cl->stats.size_rel <= 0.180910408497f ) {
                                return 747.0/657.6;
                            } else {
                                if ( cl->stats.glue <= 7.5f ) {
                                    return 1427.0/895.7;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 133296.5f ) {
                                        return 831.0/435.7;
                                    } else {
                                        return 1123.0/314.7;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                        if ( cl->stats.num_overlap_literals_rel <= 0.349410414696f ) {
                            return 983.0/811.0;
                        } else {
                            return 1456.0/383.3;
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.250839829445f ) {
                            if ( cl->stats.dump_number <= 16.5f ) {
                                return 1099.0/1061.1;
                            } else {
                                return 840.0/411.5;
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.894595623016f ) {
                                return 1076.0/423.6;
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 151.5f ) {
                                    if ( cl->stats.num_overlap_literals <= 28.5f ) {
                                        return 904.0/351.0;
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 105.5f ) {
                                            return 1228.0/254.2;
                                        } else {
                                            return 1001.0/330.8;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 0.513885974884f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 1142.0f ) {
                                            if ( cl->stats.glue_rel_long <= 1.03515863419f ) {
                                                return 916.0/211.8;
                                            } else {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.731351137161f ) {
                                                    return 1052.0/88.8;
                                                } else {
                                                    return 967.0/145.2;
                                                }
                                            }
                                        } else {
                                            return 1035.0/68.6;
                                        }
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 1.145596385f ) {
                                            return 972.0/240.1;
                                        } else {
                                            return 1214.0/225.9;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.num_total_lits_antecedents <= 122.5f ) {
                if ( rdb0_last_touched_diff <= 55181.5f ) {
                    if ( cl->stats.rdb1_last_touched_diff <= 11224.5f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 50.5f ) {
                            return 518.0/1216.4;
                        } else {
                            return 964.0/1297.1;
                        }
                    } else {
                        if ( cl->stats.dump_number <= 5.5f ) {
                            if ( cl->size() <= 14.5f ) {
                                return 768.0/930.0;
                            } else {
                                if ( cl->stats.glue <= 9.5f ) {
                                    return 995.0/911.8;
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.563074052334f ) {
                                        return 1609.0/926.0;
                                    } else {
                                        return 1394.0/389.3;
                                    }
                                }
                            }
                        } else {
                            if ( cl->size() <= 20.5f ) {
                                return 588.0/1141.8;
                            } else {
                                return 935.0/1234.6;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.glue_rel_long <= 0.872705578804f ) {
                        if ( rdb0_last_touched_diff <= 120349.0f ) {
                            if ( cl->stats.dump_number <= 11.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 45.5f ) {
                                    return 914.0/645.5;
                                } else {
                                    return 1525.0/675.8;
                                }
                            } else {
                                return 1274.0/1319.3;
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 318831.0f ) {
                                if ( cl->stats.glue_rel_long <= 0.697285056114f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.202489331365f ) {
                                        return 775.0/516.4;
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 54.5f ) {
                                            return 949.0/355.0;
                                        } else {
                                            return 966.0/433.7;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.251381158829f ) {
                                        return 1058.0/439.8;
                                    } else {
                                        if ( rdb0_act_ranking_top_10 <= 7.5f ) {
                                            return 1140.0/435.7;
                                        } else {
                                            return 1015.0/219.9;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.313293337822f ) {
                                    return 921.0/322.8;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 453186.5f ) {
                                        return 988.0/153.3;
                                    } else {
                                        return 910.0/185.6;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.119002267718f ) {
                            if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                return 1065.0/544.7;
                            } else {
                                return 1678.0/704.0;
                            }
                        } else {
                            if ( cl->stats.size_rel <= 0.940826416016f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 17.4046211243f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 49.5f ) {
                                        return 1188.0/552.7;
                                    } else {
                                        if ( rdb0_last_touched_diff <= 156730.0f ) {
                                            return 1233.0/484.2;
                                        } else {
                                            return 1148.0/201.7;
                                        }
                                    }
                                } else {
                                    return 1074.0/173.5;
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.411931097507f ) {
                                        if ( cl->stats.glue <= 12.5f ) {
                                            return 1028.0/409.5;
                                        } else {
                                            if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                                return 899.0/274.4;
                                            } else {
                                                return 1010.0/171.5;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 1.2109105587f ) {
                                            if ( cl->stats.glue_rel_queue <= 1.07995676994f ) {
                                                return 1586.0/250.1;
                                            } else {
                                                return 932.0/228.0;
                                            }
                                        } else {
                                            return 1442.0/151.3;
                                        }
                                    }
                                } else {
                                    if ( cl->size() <= 25.5f ) {
                                        return 1195.0/133.1;
                                    } else {
                                        return 1285.0/86.7;
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.glue_rel_long <= 0.875237584114f ) {
                    if ( rdb0_last_touched_diff <= 65723.0f ) {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 1.83836734295f ) {
                            return 710.0/732.3;
                        } else {
                            return 1600.0/811.0;
                        }
                    } else {
                        if ( cl->stats.num_overlap_literals_rel <= 1.24892044067f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 176425.5f ) {
                                if ( cl->stats.dump_number <= 13.5f ) {
                                    return 1199.0/280.4;
                                } else {
                                    return 982.0/397.4;
                                }
                            } else {
                                return 1422.0/326.8;
                            }
                        } else {
                            return 1482.0/637.5;
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_last_touched_diff <= 37915.0f ) {
                        if ( cl->stats.glue <= 15.5f ) {
                            if ( cl->stats.dump_number <= 2.5f ) {
                                return 892.0/266.3;
                            } else {
                                return 1136.0/540.6;
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals_rel <= 0.263962358236f ) {
                                return 865.0/413.6;
                            } else {
                                if ( cl->stats.size_rel <= 1.04573750496f ) {
                                    return 1307.0/373.2;
                                } else {
                                    if ( cl->stats.dump_number <= 3.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 1.27119493484f ) {
                                            return 1159.0/115.0;
                                        } else {
                                            return 1296.0/96.8;
                                        }
                                    } else {
                                        return 887.0/308.7;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.antec_num_total_lits_rel <= 0.648310542107f ) {
                            if ( cl->stats.num_overlap_literals <= 42.5f ) {
                                return 904.0/342.9;
                            } else {
                                if ( cl->size() <= 160.5f ) {
                                    if ( cl->stats.num_overlap_literals <= 124.5f ) {
                                        return 1851.0/397.4;
                                    } else {
                                        return 1066.0/149.3;
                                    }
                                } else {
                                    return 1123.0/115.0;
                                }
                            }
                        } else {
                            if ( cl->stats.glue <= 15.5f ) {
                                if ( cl->stats.glue_rel_long <= 1.21334385872f ) {
                                    if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                        return 1262.0/389.3;
                                    } else {
                                        if ( cl->size() <= 31.5f ) {
                                            if ( cl->stats.glue_rel_queue <= 1.07329046726f ) {
                                                return 1068.0/274.4;
                                            } else {
                                                return 893.0/205.8;
                                            }
                                        } else {
                                            return 1499.0/268.3;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 1.46107840538f ) {
                                        return 1273.0/189.6;
                                    } else {
                                        return 1031.0/90.8;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue <= 21.5f ) {
                                    if ( cl->stats.dump_number <= 12.5f ) {
                                        return 1767.0/147.3;
                                    } else {
                                        if ( cl->stats.size_rel <= 1.38270354271f ) {
                                            return 1622.0/183.6;
                                        } else {
                                            return 1624.0/314.7;
                                        }
                                    }
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 1.25186777115f ) {
                                            return 1212.0/104.9;
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 1.26058650017f ) {
                                                return 1093.0/80.7;
                                            } else {
                                                return 1166.0/28.2;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 92442.0f ) {
                                            if ( cl->stats.antec_num_total_lits_rel <= 1.50466048717f ) {
                                                return 1012.0/173.5;
                                            } else {
                                                return 1033.0/102.9;
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 1.11784589291f ) {
                                                return 1386.0/159.4;
                                            } else {
                                                if ( cl->size() <= 112.5f ) {
                                                    return 1863.0/137.2;
                                                } else {
                                                    return 1562.0/62.5;
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
}

static double estimator_should_keep_short_conf3_cluster0_3(
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
        if ( cl->stats.glue_rel_queue <= 0.789603590965f ) {
            if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                    if ( rdb0_last_touched_diff <= 9955.0f ) {
                        if ( rdb0_last_touched_diff <= 2823.5f ) {
                            if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 37.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.510576486588f ) {
                                            return 386.0/3256.0;
                                        } else {
                                            return 180.0/1892.2;
                                        }
                                    } else {
                                        return 250.0/1642.1;
                                    }
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0720162913203f ) {
                                        return 334.0/1400.0;
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 50.5f ) {
                                            if ( cl->stats.num_total_lits_antecedents <= 23.5f ) {
                                                return 348.0/2447.0;
                                            } else {
                                                return 288.0/1732.9;
                                            }
                                        } else {
                                            return 353.0/1462.6;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0373938456178f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 18.5f ) {
                                        if ( cl->stats.used_for_uip_creation <= 10.5f ) {
                                            return 214.0/1975.0;
                                        } else {
                                            return 83.0/2011.3;
                                        }
                                    } else {
                                        return 300.0/3074.4;
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 0.15828576684f ) {
                                        return 83.0/2081.9;
                                    } else {
                                        if ( rdb0_last_touched_diff <= 398.5f ) {
                                            if ( cl->stats.dump_number <= 11.5f ) {
                                                return 67.0/2023.4;
                                            } else {
                                                return 71.0/2039.5;
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.182507395744f ) {
                                                    return 99.0/2221.1;
                                                } else {
                                                    return 145.0/1843.8;
                                                }
                                            } else {
                                                if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                                                    return 146.0/1853.9;
                                                } else {
                                                    return 170.0/1942.7;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->size() <= 10.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.044737637043f ) {
                                    return 342.0/1587.6;
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 8.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 38562.0f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0349804982543f ) {
                                                return 412.0/2765.7;
                                            } else {
                                                if ( cl->stats.glue_rel_long <= 0.492256253958f ) {
                                                    if ( cl->stats.num_total_lits_antecedents <= 13.5f ) {
                                                        return 168.0/1801.5;
                                                    } else {
                                                        return 197.0/1730.9;
                                                    }
                                                } else {
                                                    return 236.0/1843.8;
                                                }
                                            }
                                        } else {
                                            return 401.0/1611.8;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                            return 478.0/3011.9;
                                        } else {
                                            return 457.0/2146.4;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->size() <= 41.5f ) {
                                    if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 16208.0f ) {
                                            return 454.0/1966.9;
                                        } else {
                                            return 843.0/2328.0;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 20860.0f ) {
                                            return 309.0/2293.7;
                                        } else {
                                            return 312.0/1644.1;
                                        }
                                    }
                                } else {
                                    return 403.0/1268.9;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.antec_num_total_lits_rel <= 0.21306142211f ) {
                            if ( rdb0_last_touched_diff <= 37173.0f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 33.5f ) {
                                    if ( cl->stats.glue <= 5.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.287295162678f ) {
                                            return 563.0/1827.7;
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.0757995918393f ) {
                                                return 354.0/1640.1;
                                            } else {
                                                if ( cl->stats.num_overlap_literals <= 5.5f ) {
                                                    return 385.0/1646.1;
                                                } else {
                                                    return 436.0/1563.4;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.117037437856f ) {
                                            return 716.0/2007.2;
                                        } else {
                                            return 577.0/2053.6;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue <= 7.5f ) {
                                        return 550.0/1480.7;
                                    } else {
                                        return 631.0/1220.5;
                                    }
                                }
                            } else {
                                if ( cl->size() <= 7.5f ) {
                                    return 524.0/1801.5;
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 13.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.101189099252f ) {
                                            return 580.0/952.2;
                                        } else {
                                            return 537.0/1026.8;
                                        }
                                    } else {
                                        return 633.0/857.4;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.size_rel <= 0.830579161644f ) {
                                if ( cl->stats.num_overlap_literals <= 84.5f ) {
                                    if ( rdb0_last_touched_diff <= 31998.5f ) {
                                        if ( cl->size() <= 12.5f ) {
                                            return 515.0/1569.5;
                                        } else {
                                            return 569.0/1212.4;
                                        }
                                    } else {
                                        return 888.0/1406.1;
                                    }
                                } else {
                                    return 529.0/1307.2;
                                }
                            } else {
                                return 1216.0/1389.9;
                            }
                        }
                    }
                } else {
                    if ( rdb0_last_touched_diff <= 35793.5f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 45.5f ) {
                            if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                                if ( cl->stats.glue <= 7.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 18.5f ) {
                                        if ( cl->stats.glue <= 4.5f ) {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.0655372366309f ) {
                                                return 509.0/1476.7;
                                            } else {
                                                return 457.0/1628.0;
                                            }
                                        } else {
                                            return 490.0/2134.3;
                                        }
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 29.5f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.100204981863f ) {
                                                return 498.0/1299.2;
                                            } else {
                                                return 460.0/1125.7;
                                            }
                                        } else {
                                            return 505.0/1418.2;
                                        }
                                    }
                                } else {
                                    return 661.0/1543.3;
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 12355.5f ) {
                                    return 283.0/1860.0;
                                } else {
                                    return 435.0/1702.6;
                                }
                            }
                        } else {
                            if ( cl->size() <= 15.5f ) {
                                return 620.0/1753.1;
                            } else {
                                if ( cl->stats.num_overlap_literals <= 49.5f ) {
                                    return 891.0/1813.6;
                                } else {
                                    return 600.0/899.7;
                                }
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 76262.5f ) {
                            if ( cl->stats.glue <= 5.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 40874.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0997973233461f ) {
                                        return 474.0/1135.8;
                                    } else {
                                        return 512.0/1075.2;
                                    }
                                } else {
                                    return 841.0/1438.4;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 6.5f ) {
                                    return 573.0/952.2;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 40234.0f ) {
                                        return 1190.0/1591.7;
                                    } else {
                                        if ( cl->stats.size_rel <= 0.541208982468f ) {
                                            return 669.0/774.7;
                                        } else {
                                            return 760.0/712.1;
                                        }
                                    }
                                }
                            }
                        } else {
                            return 2357.0/2176.7;
                        }
                    }
                }
            } else {
                if ( rdb0_last_touched_diff <= 9260.5f ) {
                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.106590613723f ) {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 7.5f ) {
                            if ( rdb0_last_touched_diff <= 2337.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0292180627584f ) {
                                    if ( rdb0_last_touched_diff <= 355.5f ) {
                                        return 81.0/2196.9;
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 3427.5f ) {
                                            return 166.0/2927.1;
                                        } else {
                                            return 165.0/1801.5;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.471590518951f ) {
                                        if ( rdb0_last_touched_diff <= 300.5f ) {
                                            return 47.0/2011.3;
                                        } else {
                                            if ( cl->stats.used_for_uip_creation <= 7.5f ) {
                                                return 193.0/3084.5;
                                            } else {
                                                return 58.0/2691.1;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 1960.5f ) {
                                            return 93.0/2049.6;
                                        } else {
                                            return 162.0/2547.9;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 23.5f ) {
                                    if ( rdb0_last_touched_diff <= 6244.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0388589948416f ) {
                                            return 195.0/1718.8;
                                        } else {
                                            if ( cl->size() <= 6.5f ) {
                                                if ( cl->stats.size_rel <= 0.183980375528f ) {
                                                    return 156.0/2134.3;
                                                } else {
                                                    return 118.0/2342.1;
                                                }
                                            } else {
                                                return 184.0/2311.9;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0759892016649f ) {
                                            return 210.0/1694.6;
                                        } else {
                                            return 157.0/1797.4;
                                        }
                                    }
                                } else {
                                    if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                        return 321.0/2374.4;
                                    } else {
                                        return 220.0/2235.2;
                                    }
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 4800.5f ) {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 19.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0642864108086f ) {
                                        if ( rdb0_last_touched_diff <= 487.0f ) {
                                            return 80.0/3028.0;
                                        } else {
                                            if ( cl->stats.num_overlap_literals <= 3.5f ) {
                                                return 82.0/2112.1;
                                            } else {
                                                if ( cl->stats.used_for_uip_creation <= 11.5f ) {
                                                    return 136.0/1936.6;
                                                } else {
                                                    return 63.0/2025.4;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.used_for_uip_creation <= 5.5f ) {
                                            return 148.0/3304.4;
                                        } else {
                                            if ( cl->size() <= 8.5f ) {
                                                if ( cl->stats.glue_rel_long <= 0.420439422131f ) {
                                                    return 41.0/3487.9;
                                                } else {
                                                    return 52.0/2352.2;
                                                }
                                            } else {
                                                return 104.0/3633.2;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 0.19327840209f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.00575229246169f ) {
                                            return 25.0/2340.1;
                                        } else {
                                            if ( cl->stats.size_rel <= 0.0402624160051f ) {
                                                return 42.0/3159.1;
                                            } else {
                                                if ( cl->stats.glue_rel_queue <= 0.201952412724f ) {
                                                    return 136.0/2449.0;
                                                } else {
                                                    if ( rdb0_last_touched_diff <= 1634.0f ) {
                                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0315592736006f ) {
                                                            return 50.0/2273.5;
                                                        } else {
                                                            if ( cl->stats.size_rel <= 0.129256695509f ) {
                                                                return 18.0/2207.0;
                                                            } else {
                                                                return 43.0/2620.5;
                                                            }
                                                        }
                                                    } else {
                                                        return 88.0/2079.9;
                                                    }
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.glue <= 8.5f ) {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.077017813921f ) {
                                                return 44.0/2065.7;
                                            } else {
                                                if ( cl->stats.size_rel <= 0.377833247185f ) {
                                                    if ( cl->stats.glue_rel_queue <= 0.406268268824f ) {
                                                        return 17.0/2644.7;
                                                    } else {
                                                        return 25.0/2073.8;
                                                    }
                                                } else {
                                                    return 40.0/2108.1;
                                                }
                                            }
                                        } else {
                                            return 32.0/4064.9;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 15.5f ) {
                                    return 177.0/3469.8;
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.152154624462f ) {
                                        return 173.0/2037.5;
                                    } else {
                                        return 111.0/1910.4;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                            if ( cl->stats.used_for_uip_creation <= 8.5f ) {
                                if ( cl->stats.dump_number <= 1.5f ) {
                                    return 292.0/2291.7;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 2900.5f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.0949447900057f ) {
                                            return 197.0/2037.5;
                                        } else {
                                            if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                                return 181.0/2067.8;
                                            } else {
                                                if ( cl->stats.dump_number <= 13.5f ) {
                                                    return 112.0/2166.6;
                                                } else {
                                                    return 114.0/1825.7;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 6.5f ) {
                                            if ( cl->stats.num_total_lits_antecedents <= 42.5f ) {
                                                return 273.0/2453.1;
                                            } else {
                                                return 255.0/1658.2;
                                            }
                                        } else {
                                            return 131.0/1946.7;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 4.5f ) {
                                    return 166.0/2913.0;
                                } else {
                                    if ( rdb0_last_touched_diff <= 4059.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0467534549534f ) {
                                            if ( cl->stats.num_total_lits_antecedents <= 23.5f ) {
                                                return 44.0/2120.2;
                                            } else {
                                                return 97.0/2475.3;
                                            }
                                        } else {
                                            if ( cl->size() <= 10.5f ) {
                                                return 51.0/3956.0;
                                            } else {
                                                if ( cl->size() <= 21.5f ) {
                                                    return 94.0/2348.2;
                                                } else {
                                                    return 71.0/3401.2;
                                                }
                                            }
                                        }
                                    } else {
                                        return 141.0/1799.5;
                                    }
                                }
                            }
                        } else {
                            return 275.0/2328.0;
                        }
                    }
                } else {
                    if ( cl->stats.size_rel <= 0.639577567577f ) {
                        if ( rdb0_last_touched_diff <= 13998.5f ) {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 5.5f ) {
                                if ( cl->stats.glue_rel_queue <= 0.298868596554f ) {
                                    return 338.0/1369.8;
                                } else {
                                    if ( cl->size() <= 8.5f ) {
                                        if ( cl->stats.dump_number <= 11.5f ) {
                                            return 256.0/1535.2;
                                        } else {
                                            return 261.0/1839.8;
                                        }
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.228298604488f ) {
                                            return 319.0/1561.4;
                                        } else {
                                            return 414.0/1426.2;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0273370780051f ) {
                                    return 343.0/2100.0;
                                } else {
                                    if ( cl->stats.glue <= 5.5f ) {
                                        return 289.0/3052.2;
                                    } else {
                                        return 387.0/2596.3;
                                    }
                                }
                            }
                        } else {
                            if ( cl->size() <= 11.5f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.117210254073f ) {
                                    if ( rdb0_last_touched_diff <= 15727.5f ) {
                                        return 342.0/1444.4;
                                    } else {
                                        return 516.0/2497.4;
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.233009546995f ) {
                                        return 248.0/1591.7;
                                    } else {
                                        return 282.0/1517.0;
                                    }
                                }
                            } else {
                                return 818.0/2354.2;
                            }
                        }
                    } else {
                        if ( cl->size() <= 22.5f ) {
                            return 409.0/1494.8;
                        } else {
                            return 659.0/1851.9;
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                if ( cl->stats.antecedents_glue_long_reds_var <= 9.00186729431f ) {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                        if ( cl->stats.antec_num_total_lits_rel <= 0.462421476841f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.glue <= 7.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.857260286808f ) {
                                        return 449.0/1184.2;
                                    } else {
                                        return 419.0/1470.6;
                                    }
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.188841879368f ) {
                                        return 927.0/1749.0;
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 25455.0f ) {
                                            if ( cl->stats.glue_rel_queue <= 0.94944268465f ) {
                                                return 536.0/1067.2;
                                            } else {
                                                return 661.0/986.5;
                                            }
                                        } else {
                                            return 766.0/794.8;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 17.5f ) {
                                    return 1086.0/1690.5;
                                } else {
                                    return 1177.0/887.6;
                                }
                            }
                        } else {
                            if ( cl->stats.glue <= 9.5f ) {
                                if ( cl->stats.glue_rel_queue <= 0.97849214077f ) {
                                    return 987.0/1545.3;
                                } else {
                                    return 623.0/823.1;
                                }
                            } else {
                                if ( cl->stats.dump_number <= 10.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.967501878738f ) {
                                        return 779.0/597.1;
                                    } else {
                                        if ( cl->stats.glue <= 13.5f ) {
                                            return 1021.0/490.2;
                                        } else {
                                            return 1003.0/262.3;
                                        }
                                    }
                                } else {
                                    return 684.0/776.7;
                                }
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 15993.5f ) {
                            if ( cl->stats.dump_number <= 2.5f ) {
                                return 514.0/1299.2;
                            } else {
                                if ( cl->stats.num_overlap_literals <= 17.5f ) {
                                    return 294.0/1468.6;
                                } else {
                                    return 372.0/1434.3;
                                }
                            }
                        } else {
                            return 556.0/1470.6;
                        }
                    }
                } else {
                    if ( cl->stats.glue_rel_long <= 1.05613422394f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 26728.5f ) {
                            if ( cl->stats.dump_number <= 3.5f ) {
                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                    return 1106.0/1252.8;
                                } else {
                                    return 908.0/492.2;
                                }
                            } else {
                                return 579.0/1248.7;
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 253.5f ) {
                                return 1485.0/958.2;
                            } else {
                                return 903.0/345.0;
                            }
                        }
                    } else {
                        if ( cl->stats.glue <= 21.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 40507.5f ) {
                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                    return 954.0/752.5;
                                } else {
                                    return 1168.0/589.1;
                                }
                            } else {
                                return 1016.0/258.2;
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 43913.5f ) {
                                if ( cl->stats.num_overlap_literals <= 362.5f ) {
                                    return 1025.0/466.0;
                                } else {
                                    return 1012.0/131.1;
                                }
                            } else {
                                return 1170.0/88.8;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                    if ( cl->size() <= 13.5f ) {
                        if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                            if ( cl->stats.glue <= 7.5f ) {
                                return 299.0/2959.4;
                            } else {
                                return 232.0/1708.7;
                            }
                        } else {
                            return 567.0/2313.9;
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 878.0f ) {
                            return 264.0/1775.2;
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 11097.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 147.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.263089805841f ) {
                                        return 338.0/1593.7;
                                    } else {
                                        return 356.0/2445.0;
                                    }
                                } else {
                                    return 377.0/1357.7;
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 52998.5f ) {
                                    if ( cl->stats.num_overlap_literals <= 31.5f ) {
                                        return 370.0/1285.0;
                                    } else {
                                        return 581.0/1319.3;
                                    }
                                } else {
                                    return 591.0/1047.0;
                                }
                            }
                        }
                    }
                } else {
                    if ( rdb0_last_touched_diff <= 3264.5f ) {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 9.5f ) {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                if ( cl->size() <= 38.5f ) {
                                    return 265.0/3389.1;
                                } else {
                                    return 189.0/1896.3;
                                }
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 11.5f ) {
                                    return 253.0/3483.9;
                                } else {
                                    return 80.0/2311.9;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 1101.0f ) {
                                if ( rdb0_last_touched_diff <= 418.0f ) {
                                    return 23.0/2634.6;
                                } else {
                                    return 78.0/3056.2;
                                }
                            } else {
                                return 106.0/3266.0;
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_long <= 0.993904709816f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.689429044724f ) {
                                return 213.0/1710.7;
                            } else {
                                return 279.0/1634.0;
                            }
                        } else {
                            return 337.0/3072.4;
                        }
                    }
                }
            }
        }
    } else {
        if ( cl->stats.antecedents_glue_long_reds_var <= 1.01072311401f ) {
            if ( cl->stats.rdb1_last_touched_diff <= 55744.5f ) {
                if ( rdb0_last_touched_diff <= 30013.5f ) {
                    if ( cl->stats.num_total_lits_antecedents <= 29.5f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 19.5f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                return 232.0/1676.4;
                            } else {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                    if ( cl->stats.size_rel <= 0.171918332577f ) {
                                        return 474.0/2021.4;
                                    } else {
                                        return 698.0/2213.0;
                                    }
                                } else {
                                    return 326.0/1736.9;
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.590962767601f ) {
                                return 585.0/2198.9;
                            } else {
                                return 557.0/1442.4;
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 11697.5f ) {
                            if ( rdb0_last_touched_diff <= 8569.0f ) {
                                return 296.0/1646.1;
                            } else {
                                return 416.0/1440.4;
                            }
                        } else {
                            if ( cl->stats.dump_number <= 2.5f ) {
                                if ( cl->stats.size_rel <= 0.610127151012f ) {
                                    return 1071.0/1630.0;
                                } else {
                                    return 1087.0/841.2;
                                }
                            } else {
                                if ( cl->stats.size_rel <= 0.668187499046f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.135052025318f ) {
                                        return 501.0/1133.7;
                                    } else {
                                        return 490.0/2003.2;
                                    }
                                } else {
                                    return 541.0/1065.1;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->size() <= 10.5f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 18.5f ) {
                            if ( cl->stats.glue <= 4.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0335393548012f ) {
                                    return 728.0/1335.5;
                                } else {
                                    return 639.0/1555.4;
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 35996.0f ) {
                                    return 437.0/1383.9;
                                } else {
                                    return 467.0/1083.3;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 38949.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 26787.5f ) {
                                    return 465.0/1137.8;
                                } else {
                                    return 780.0/1535.2;
                                }
                            } else {
                                return 1029.0/1448.4;
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_queue <= 0.608607411385f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 70.5f ) {
                                if ( cl->stats.glue_rel_long <= 0.436957776546f ) {
                                    return 607.0/1190.2;
                                } else {
                                    return 817.0/1115.6;
                                }
                            } else {
                                return 651.0/873.5;
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 148.5f ) {
                                if ( cl->stats.dump_number <= 9.5f ) {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                                        return 1077.0/1018.7;
                                    } else {
                                        if ( rdb0_last_touched_diff <= 48768.5f ) {
                                            return 855.0/722.2;
                                        } else {
                                            return 891.0/548.7;
                                        }
                                    }
                                } else {
                                    return 876.0/1420.2;
                                }
                            } else {
                                return 901.0/401.4;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.size_rel <= 0.594701170921f ) {
                    if ( cl->stats.glue <= 7.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 118828.0f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 32.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.257192313671f ) {
                                    if ( rdb0_last_touched_diff <= 84696.0f ) {
                                        if ( cl->stats.glue <= 4.5f ) {
                                            return 657.0/930.0;
                                        } else {
                                            return 636.0/1139.8;
                                        }
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.11225028336f ) {
                                            return 1028.0/1478.7;
                                        } else {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0717191547155f ) {
                                                return 674.0/700.0;
                                            } else {
                                                return 689.0/891.7;
                                            }
                                        }
                                    }
                                } else {
                                    return 758.0/790.8;
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 82768.5f ) {
                                    return 698.0/722.2;
                                } else {
                                    return 735.0/667.7;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 266486.0f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.148373246193f ) {
                                    if ( cl->size() <= 8.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 149519.5f ) {
                                            return 803.0/1008.7;
                                        } else {
                                            if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                                return 669.0/851.3;
                                            } else {
                                                return 1105.0/921.9;
                                            }
                                        }
                                    } else {
                                        return 1192.0/839.2;
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 27.5f ) {
                                        return 1496.0/845.3;
                                    } else {
                                        return 934.0/808.9;
                                    }
                                }
                            } else {
                                if ( cl->stats.size_rel <= 0.137026697397f ) {
                                    return 1248.0/1022.8;
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.506145119667f ) {
                                        return 1518.0/591.1;
                                    } else {
                                        return 1212.0/659.7;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.0407100245357f ) {
                            if ( cl->size() <= 10.5f ) {
                                return 761.0/726.2;
                            } else {
                                if ( cl->stats.dump_number <= 30.5f ) {
                                    if ( cl->stats.num_overlap_literals <= 20.5f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 35.5f ) {
                                            return 776.0/490.2;
                                        } else {
                                            return 728.0/649.6;
                                        }
                                    } else {
                                        return 727.0/643.5;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0462450832129f ) {
                                        return 915.0/363.1;
                                    } else {
                                        return 884.0/421.6;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals <= 19.5f ) {
                                return 1032.0/621.3;
                            } else {
                                return 1538.0/496.3;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_antecedents_rel <= 0.459424227476f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 26.5f ) {
                            return 1489.0/1045.0;
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.899446368217f ) {
                                if ( rdb0_act_ranking_top_10 <= 7.5f ) {
                                    if ( cl->stats.dump_number <= 34.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 105300.5f ) {
                                            return 757.0/591.1;
                                        } else {
                                            return 1035.0/496.3;
                                        }
                                    } else {
                                        return 913.0/451.9;
                                    }
                                } else {
                                    if ( cl->size() <= 25.5f ) {
                                        return 970.0/381.3;
                                    } else {
                                        return 824.0/421.6;
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 143567.5f ) {
                                    return 1388.0/704.0;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 226595.0f ) {
                                        return 955.0/225.9;
                                    } else {
                                        return 1238.0/349.0;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.num_antecedents_rel <= 0.942021846771f ) {
                            if ( cl->stats.glue <= 8.5f ) {
                                return 1355.0/538.6;
                            } else {
                                if ( rdb0_last_touched_diff <= 151612.5f ) {
                                    return 1008.0/423.6;
                                } else {
                                    return 1703.0/385.3;
                                }
                            }
                        } else {
                            return 1660.0/359.1;
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.glue_rel_long <= 0.876702308655f ) {
                if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                    if ( cl->size() <= 16.5f ) {
                        if ( cl->stats.glue_rel_queue <= 0.761663079262f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 21008.0f ) {
                                return 424.0/1279.0;
                            } else {
                                return 1207.0/1571.5;
                            }
                        } else {
                            return 1021.0/1079.3;
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 245.5f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                                return 1172.0/1373.8;
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.458420902491f ) {
                                    return 1104.0/784.7;
                                } else {
                                    return 696.0/643.5;
                                }
                            }
                        } else {
                            return 1031.0/476.1;
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_last_touched_diff <= 95381.0f ) {
                        if ( cl->stats.num_overlap_literals <= 170.5f ) {
                            if ( cl->stats.glue <= 9.5f ) {
                                if ( rdb0_last_touched_diff <= 65223.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 59.5f ) {
                                        return 602.0/798.9;
                                    } else {
                                        return 731.0/629.4;
                                    }
                                } else {
                                    return 1348.0/1026.8;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 35.5f ) {
                                    return 924.0/655.6;
                                } else {
                                    return 1371.0/748.4;
                                }
                            }
                        } else {
                            return 1451.0/486.2;
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 185648.0f ) {
                            if ( cl->stats.size_rel <= 0.87197124958f ) {
                                if ( cl->stats.dump_number <= 18.5f ) {
                                    if ( cl->stats.glue <= 9.5f ) {
                                        return 1452.0/839.2;
                                    } else {
                                        return 1178.0/381.3;
                                    }
                                } else {
                                    return 970.0/857.4;
                                }
                            } else {
                                if ( cl->stats.dump_number <= 15.5f ) {
                                    return 1148.0/232.0;
                                } else {
                                    return 937.0/377.2;
                                }
                            }
                        } else {
                            if ( cl->stats.num_antecedents_rel <= 1.21729266644f ) {
                                if ( cl->stats.size_rel <= 0.675191879272f ) {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 8.5f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 51.5f ) {
                                            return 1033.0/548.7;
                                        } else {
                                            return 1422.0/478.1;
                                        }
                                    } else {
                                        return 1003.0/308.7;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.713141679764f ) {
                                        return 1629.0/468.0;
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.472263336182f ) {
                                            return 1474.0/246.1;
                                        } else {
                                            return 982.0/215.9;
                                        }
                                    }
                                }
                            } else {
                                return 849.0/377.2;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.glue <= 11.5f ) {
                    if ( cl->stats.rdb1_last_touched_diff <= 45622.5f ) {
                        if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                            if ( cl->size() <= 19.5f ) {
                                return 1275.0/1408.1;
                            } else {
                                return 907.0/574.9;
                            }
                        } else {
                            return 1201.0/486.2;
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 52.5f ) {
                            if ( cl->stats.glue <= 9.5f ) {
                                return 939.0/544.7;
                            } else {
                                return 977.0/334.9;
                            }
                        } else {
                            if ( cl->stats.glue <= 9.5f ) {
                                if ( rdb0_last_touched_diff <= 119231.0f ) {
                                    return 1093.0/516.4;
                                } else {
                                    return 1726.0/504.3;
                                }
                            } else {
                                if ( cl->stats.dump_number <= 30.5f ) {
                                    if ( cl->stats.glue_rel_long <= 1.12789940834f ) {
                                        if ( cl->stats.glue_rel_long <= 0.969755768776f ) {
                                            return 919.0/282.4;
                                        } else {
                                            return 1343.0/324.8;
                                        }
                                    } else {
                                        return 1039.0/173.5;
                                    }
                                } else {
                                    return 862.0/250.1;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                        if ( cl->stats.num_antecedents_rel <= 0.37067759037f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 219.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 17.6215286255f ) {
                                    return 771.0/486.2;
                                } else {
                                    return 952.0/397.4;
                                }
                            } else {
                                return 1073.0/207.8;
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals <= 334.5f ) {
                                if ( rdb0_last_touched_diff <= 43670.0f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 1.13142490387f ) {
                                        return 1380.0/411.5;
                                    } else {
                                        return 1024.0/383.3;
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 1.08378493786f ) {
                                        return 994.0/183.6;
                                    } else {
                                        return 1114.0/123.1;
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 37924.0f ) {
                                    return 1377.0/191.6;
                                } else {
                                    return 1287.0/84.7;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_long <= 1.33610367775f ) {
                            if ( cl->stats.size_rel <= 1.16937446594f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 144.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 55227.0f ) {
                                        return 929.0/381.3;
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 73.5f ) {
                                            return 1709.0/393.4;
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 175356.0f ) {
                                                return 1378.0/292.5;
                                            } else {
                                                return 1293.0/153.3;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue <= 43.5f ) {
                                        if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                            if ( cl->stats.antec_num_total_lits_rel <= 2.16297578812f ) {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.718047618866f ) {
                                                    return 989.0/151.3;
                                                } else {
                                                    if ( cl->stats.glue <= 16.5f ) {
                                                        return 1084.0/115.0;
                                                    } else {
                                                        return 1123.0/46.4;
                                                    }
                                                }
                                            } else {
                                                return 909.0/159.4;
                                            }
                                        } else {
                                            if ( cl->stats.size_rel <= 0.85463142395f ) {
                                                return 1841.0/338.9;
                                            } else {
                                                return 1073.0/131.1;
                                            }
                                        }
                                    } else {
                                        return 1608.0/403.5;
                                    }
                                }
                            } else {
                                if ( cl->stats.dump_number <= 40.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 697.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 49269.5f ) {
                                            return 967.0/191.6;
                                        } else {
                                            if ( cl->size() <= 28.5f ) {
                                                return 1407.0/117.0;
                                            } else {
                                                if ( cl->stats.size_rel <= 2.05032014847f ) {
                                                    if ( cl->stats.glue_rel_queue <= 1.00020003319f ) {
                                                        return 947.0/193.7;
                                                    } else {
                                                        if ( cl->stats.glue <= 21.5f ) {
                                                            return 1658.0/244.1;
                                                        } else {
                                                            return 1322.0/100.9;
                                                        }
                                                    }
                                                } else {
                                                    return 992.0/86.7;
                                                }
                                            }
                                        }
                                    } else {
                                        return 2048.0/137.2;
                                    }
                                } else {
                                    return 1152.0/276.4;
                                }
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 326.5f ) {
                                if ( rdb0_act_ranking_top_10 <= 7.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 48745.0f ) {
                                        return 946.0/191.6;
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 187.5f ) {
                                            if ( cl->stats.dump_number <= 16.5f ) {
                                                return 1205.0/86.7;
                                            } else {
                                                return 1014.0/123.1;
                                            }
                                        } else {
                                            return 1232.0/173.5;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 1.55582070351f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 151.5f ) {
                                            return 1119.0/104.9;
                                        } else {
                                            return 1045.0/137.2;
                                        }
                                    } else {
                                        return 1172.0/64.6;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 91547.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 1.54180562496f ) {
                                        return 1391.0/159.4;
                                    } else {
                                        return 1343.0/100.9;
                                    }
                                } else {
                                    if ( cl->stats.glue <= 32.5f ) {
                                        return 1552.0/84.7;
                                    } else {
                                        return 1608.0/46.4;
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

static double estimator_should_keep_short_conf3_cluster0_4(
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
        if ( cl->stats.num_overlap_literals <= 40.5f ) {
            if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                    if ( rdb0_last_touched_diff <= 9182.0f ) {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.0863223150373f ) {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                                    if ( rdb0_last_touched_diff <= 1822.0f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0622588843107f ) {
                                            return 260.0/2574.1;
                                        } else {
                                            if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                                return 247.0/2350.2;
                                            } else {
                                                return 141.0/3740.1;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.282153785229f ) {
                                            return 423.0/2037.5;
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 24470.0f ) {
                                                if ( cl->stats.glue <= 5.5f ) {
                                                    if ( cl->stats.size_rel <= 0.215737015009f ) {
                                                        return 180.0/1779.3;
                                                    } else {
                                                        return 182.0/1730.9;
                                                    }
                                                } else {
                                                    return 456.0/2812.1;
                                                }
                                            } else {
                                                return 506.0/2515.6;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue <= 7.5f ) {
                                        if ( rdb0_last_touched_diff <= 1417.5f ) {
                                            return 331.0/3177.3;
                                        } else {
                                            if ( cl->stats.dump_number <= 20.5f ) {
                                                return 433.0/2092.0;
                                            } else {
                                                return 409.0/1559.4;
                                            }
                                        }
                                    } else {
                                        return 401.0/3030.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.0683799386024f ) {
                                    return 311.0/2261.4;
                                } else {
                                    if ( rdb0_last_touched_diff <= 1388.0f ) {
                                        return 192.0/3350.8;
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 7.5f ) {
                                            if ( cl->stats.glue <= 4.5f ) {
                                                return 155.0/1813.6;
                                            } else {
                                                return 179.0/1787.3;
                                            }
                                        } else {
                                            return 266.0/1896.3;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 14339.5f ) {
                                if ( cl->stats.num_overlap_literals <= 7.5f ) {
                                    return 243.0/2622.5;
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 36.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.104347042739f ) {
                                            return 219.0/1599.7;
                                        } else {
                                            return 182.0/1886.2;
                                        }
                                    } else {
                                        if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                            return 397.0/1898.3;
                                        } else {
                                            return 150.0/1811.6;
                                        }
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 964.0f ) {
                                    if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                        return 298.0/1837.8;
                                    } else {
                                        return 119.0/2029.4;
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 3199.0f ) {
                                        if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                            return 458.0/1511.0;
                                        } else {
                                            return 256.0/2215.0;
                                        }
                                    } else {
                                        if ( cl->stats.dump_number <= 7.5f ) {
                                            return 329.0/1551.3;
                                        } else {
                                            if ( cl->size() <= 10.5f ) {
                                                return 367.0/1504.9;
                                            } else {
                                                return 696.0/2063.7;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 32.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 19437.5f ) {
                                if ( cl->size() <= 10.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.314456880093f ) {
                                        return 606.0/2114.2;
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.230468183756f ) {
                                            if ( cl->size() <= 6.5f ) {
                                                return 404.0/2537.8;
                                            } else {
                                                if ( cl->stats.glue_rel_long <= 0.531329512596f ) {
                                                    return 324.0/1426.2;
                                                } else {
                                                    return 361.0/1480.7;
                                                }
                                            }
                                        } else {
                                            return 366.0/1379.8;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.122449345887f ) {
                                        return 501.0/1345.6;
                                    } else {
                                        return 466.0/1396.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 56894.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0440621450543f ) {
                                        return 480.0/1690.5;
                                    } else {
                                        if ( rdb0_last_touched_diff <= 33917.0f ) {
                                            return 449.0/1291.1;
                                        } else {
                                            if ( cl->size() <= 8.5f ) {
                                                return 422.0/1343.5;
                                            } else {
                                                return 712.0/1492.8;
                                            }
                                        }
                                    }
                                } else {
                                    return 669.0/1264.9;
                                }
                            }
                        } else {
                            if ( cl->stats.glue <= 7.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 22593.5f ) {
                                    if ( cl->stats.size_rel <= 0.4100497365f ) {
                                        return 454.0/1396.0;
                                    } else {
                                        return 489.0/1087.3;
                                    }
                                } else {
                                    return 661.0/1151.9;
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 46257.0f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 16360.0f ) {
                                        if ( cl->stats.dump_number <= 2.5f ) {
                                            return 627.0/859.4;
                                        } else {
                                            return 752.0/1960.8;
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 20.5f ) {
                                            return 591.0/1020.8;
                                        } else {
                                            return 642.0/819.0;
                                        }
                                    }
                                } else {
                                    return 1267.0/1069.2;
                                }
                            }
                        }
                    }
                } else {
                    if ( rdb0_last_touched_diff <= 45627.0f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 45.5f ) {
                            if ( cl->stats.glue <= 9.5f ) {
                                if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.0541738979518f ) {
                                        return 760.0/1482.7;
                                    } else {
                                        if ( cl->size() <= 11.5f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 20822.5f ) {
                                                if ( cl->stats.glue_rel_queue <= 0.304263412952f ) {
                                                    return 417.0/1252.8;
                                                } else {
                                                    if ( cl->size() <= 8.5f ) {
                                                        if ( cl->stats.rdb1_last_touched_diff <= 14185.5f ) {
                                                            return 639.0/2876.7;
                                                        } else {
                                                            return 382.0/1343.5;
                                                        }
                                                    } else {
                                                        return 610.0/2045.6;
                                                    }
                                                }
                                            } else {
                                                if ( cl->size() <= 7.5f ) {
                                                    return 672.0/1837.8;
                                                } else {
                                                    return 577.0/1315.3;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.231111109257f ) {
                                                return 548.0/1157.9;
                                            } else {
                                                return 712.0/1115.6;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                                        return 432.0/1652.2;
                                    } else {
                                        return 316.0/2156.5;
                                    }
                                }
                            } else {
                                return 836.0/1486.8;
                            }
                        } else {
                            if ( cl->stats.glue <= 7.5f ) {
                                return 696.0/1502.9;
                            } else {
                                if ( rdb0_last_touched_diff <= 13701.5f ) {
                                    return 486.0/1083.3;
                                } else {
                                    if ( cl->stats.size_rel <= 0.937622070312f ) {
                                        return 899.0/1172.1;
                                    } else {
                                        return 719.0/655.6;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_queue <= 0.78723692894f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 21.5f ) {
                                if ( cl->stats.dump_number <= 37.5f ) {
                                    return 1089.0/1660.3;
                                } else {
                                    return 545.0/1095.4;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 12.5f ) {
                                    return 1080.0/990.5;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 61524.5f ) {
                                        return 817.0/1266.9;
                                    } else {
                                        return 837.0/766.6;
                                    }
                                }
                            }
                        } else {
                            if ( cl->size() <= 17.5f ) {
                                return 694.0/804.9;
                            } else {
                                return 890.0/480.1;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.rdb1_used_for_uip_creation <= 6.5f ) {
                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.0943827182055f ) {
                        if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 20.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.0351068414748f ) {
                                    return 283.0/1511.0;
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 4.5f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0308333411813f ) {
                                                return 414.0/2483.3;
                                            } else {
                                                if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                                    return 273.0/2025.4;
                                                } else {
                                                    return 330.0/3126.8;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.0830618441105f ) {
                                                return 138.0/1805.5;
                                            } else {
                                                return 144.0/1795.4;
                                            }
                                        }
                                    } else {
                                        return 496.0/2862.6;
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 10413.5f ) {
                                    if ( cl->stats.num_overlap_literals <= 12.5f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.15081268549f ) {
                                            return 268.0/1611.8;
                                        } else {
                                            return 210.0/1688.5;
                                        }
                                    } else {
                                        return 260.0/2315.9;
                                    }
                                } else {
                                    if ( cl->size() <= 12.5f ) {
                                        return 480.0/2065.7;
                                    } else {
                                        return 577.0/1837.8;
                                    }
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 2456.5f ) {
                                if ( rdb0_last_touched_diff <= 354.5f ) {
                                    if ( cl->stats.used_for_uip_creation <= 12.5f ) {
                                        return 81.0/2045.6;
                                    } else {
                                        return 32.0/2255.4;
                                    }
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0593066811562f ) {
                                        return 206.0/3032.0;
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 4244.0f ) {
                                            if ( cl->stats.used_for_uip_creation <= 10.5f ) {
                                                return 99.0/2200.9;
                                            } else {
                                                return 61.0/1995.1;
                                            }
                                        } else {
                                            return 60.0/2003.2;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0321548879147f ) {
                                        return 211.0/1621.9;
                                    } else {
                                        return 216.0/2779.9;
                                    }
                                } else {
                                    return 209.0/3102.6;
                                }
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 9905.5f ) {
                            if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                if ( cl->size() <= 9.5f ) {
                                    return 243.0/2769.8;
                                } else {
                                    if ( cl->stats.dump_number <= 5.5f ) {
                                        return 308.0/1708.7;
                                    } else {
                                        return 353.0/2687.1;
                                    }
                                }
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 8.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.553287446499f ) {
                                        return 159.0/1839.8;
                                    } else {
                                        return 248.0/2178.7;
                                    }
                                } else {
                                    if ( cl->size() <= 12.5f ) {
                                        return 99.0/2447.0;
                                    } else {
                                        return 131.0/1886.2;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.331418097019f ) {
                                if ( cl->stats.dump_number <= 1.5f ) {
                                    return 491.0/1103.5;
                                } else {
                                    if ( cl->stats.glue <= 5.5f ) {
                                        return 300.0/1494.8;
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 4492.5f ) {
                                            return 489.0/1956.8;
                                        } else {
                                            return 469.0/1412.1;
                                        }
                                    }
                                }
                            } else {
                                return 462.0/1200.3;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 13.5f ) {
                        if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                            if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 1906.5f ) {
                                    return 237.0/1827.7;
                                } else {
                                    if ( rdb0_last_touched_diff <= 14377.0f ) {
                                        return 296.0/1517.0;
                                    } else {
                                        return 391.0/1751.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue <= 7.5f ) {
                                    if ( rdb0_last_touched_diff <= 4710.0f ) {
                                        if ( cl->stats.size_rel <= 0.25994476676f ) {
                                            if ( cl->size() <= 5.5f ) {
                                                return 146.0/3393.1;
                                            } else {
                                                return 129.0/2249.3;
                                            }
                                        } else {
                                            return 112.0/3943.9;
                                        }
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.1141641289f ) {
                                            return 153.0/1767.2;
                                        } else {
                                            return 151.0/2043.5;
                                        }
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 2387.0f ) {
                                        return 121.0/1839.8;
                                    } else {
                                        return 235.0/2144.4;
                                    }
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 3645.0f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.0490325316787f ) {
                                    return 88.0/2420.8;
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.474568068981f ) {
                                        if ( cl->stats.used_for_uip_creation <= 21.5f ) {
                                            return 53.0/2662.9;
                                        } else {
                                            return 35.0/2150.5;
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0770861804485f ) {
                                            return 50.0/2061.7;
                                        } else {
                                            return 58.0/1964.9;
                                        }
                                    }
                                }
                            } else {
                                return 124.0/1827.7;
                            }
                        }
                    } else {
                        if ( cl->stats.size_rel <= 0.0373499691486f ) {
                            if ( cl->stats.dump_number <= 11.5f ) {
                                return 29.0/2860.6;
                            } else {
                                return 46.0/2134.3;
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.207089573145f ) {
                                if ( cl->stats.glue_rel_queue <= 0.144659519196f ) {
                                    return 162.0/2265.5;
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 22.5f ) {
                                        return 154.0/1918.5;
                                    } else {
                                        return 57.0/2251.3;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 2901.5f ) {
                                    if ( rdb0_last_touched_diff <= 5237.5f ) {
                                        if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                            if ( cl->stats.num_antecedents_rel <= 0.106172904372f ) {
                                                if ( cl->stats.num_overlap_literals_rel <= 0.0407452881336f ) {
                                                    if ( cl->stats.num_overlap_literals_rel <= 0.0189326517284f ) {
                                                        if ( cl->stats.size_rel <= 0.0874955803156f ) {
                                                            return 50.0/2085.9;
                                                        } else {
                                                            return 114.0/3118.8;
                                                        }
                                                    } else {
                                                        return 63.0/3191.4;
                                                    }
                                                } else {
                                                    return 85.0/1918.5;
                                                }
                                            } else {
                                                if ( cl->stats.dump_number <= 24.5f ) {
                                                    if ( cl->stats.antec_num_total_lits_rel <= 0.104723021388f ) {
                                                        return 26.0/2366.3;
                                                    } else {
                                                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.173749998212f ) {
                                                            if ( cl->stats.used_for_uip_creation <= 16.5f ) {
                                                                return 46.0/2469.2;
                                                            } else {
                                                                return 17.0/2164.6;
                                                            }
                                                        } else {
                                                            return 55.0/2100.0;
                                                        }
                                                    }
                                                } else {
                                                    return 89.0/3461.7;
                                                }
                                            }
                                        } else {
                                            if ( rdb0_last_touched_diff <= 1593.5f ) {
                                                if ( cl->stats.size_rel <= 0.188602626324f ) {
                                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0440827161074f ) {
                                                        return 35.0/2430.9;
                                                    } else {
                                                        return 42.0/1989.1;
                                                    }
                                                } else {
                                                    if ( cl->stats.dump_number <= 20.5f ) {
                                                        if ( cl->stats.glue_rel_long <= 0.490242004395f ) {
                                                            return 10.0/2092.0;
                                                        } else {
                                                            return 16.0/2457.1;
                                                        }
                                                    } else {
                                                        return 26.0/2033.5;
                                                    }
                                                }
                                            } else {
                                                return 60.0/2319.9;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.231111109257f ) {
                                            if ( rdb0_last_touched_diff <= 8281.0f ) {
                                                return 134.0/2287.6;
                                            } else {
                                                return 254.0/2434.9;
                                            }
                                        } else {
                                            return 226.0/1605.8;
                                        }
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 9979.0f ) {
                                        if ( cl->size() <= 5.5f ) {
                                            return 62.0/2156.5;
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.12318585813f ) {
                                                return 131.0/2285.6;
                                            } else {
                                                return 97.0/2872.7;
                                            }
                                        }
                                    } else {
                                        return 315.0/1692.5;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                    if ( cl->stats.num_total_lits_antecedents <= 361.5f ) {
                        if ( cl->stats.glue_rel_queue <= 0.907349586487f ) {
                            if ( cl->stats.glue <= 7.5f ) {
                                if ( rdb0_last_touched_diff <= 35477.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.562006831169f ) {
                                        return 440.0/1226.5;
                                    } else {
                                        return 532.0/1309.2;
                                    }
                                } else {
                                    return 904.0/1240.7;
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 38538.5f ) {
                                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                        return 1037.0/1962.9;
                                    } else {
                                        return 768.0/984.5;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.789284944534f ) {
                                        return 1261.0/1305.2;
                                    } else {
                                        return 927.0/589.1;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 1.04303634167f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 51363.5f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 6.81829023361f ) {
                                        return 877.0/1089.4;
                                    } else {
                                        return 1292.0/970.3;
                                    }
                                } else {
                                    return 859.0/345.0;
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 40932.0f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 8837.5f ) {
                                        return 993.0/484.2;
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.733572125435f ) {
                                            return 838.0/643.5;
                                        } else {
                                            return 1089.0/562.8;
                                        }
                                    }
                                } else {
                                    return 1553.0/361.1;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 20.4145965576f ) {
                            if ( cl->stats.glue_rel_queue <= 0.944974899292f ) {
                                return 1196.0/1218.5;
                            } else {
                                return 1764.0/716.1;
                            }
                        } else {
                            if ( cl->stats.glue <= 32.5f ) {
                                return 940.0/258.2;
                            } else {
                                return 1693.0/223.9;
                            }
                        }
                    }
                } else {
                    if ( rdb0_last_touched_diff <= 1973.5f ) {
                        if ( cl->stats.glue_rel_queue <= 0.883096575737f ) {
                            if ( cl->stats.num_overlap_literals <= 77.5f ) {
                                return 238.0/1829.7;
                            } else {
                                return 195.0/1900.3;
                            }
                        } else {
                            return 393.0/2019.3;
                        }
                    } else {
                        if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                return 615.0/2279.6;
                            } else {
                                return 1060.0/2011.3;
                            }
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 5.5f ) {
                                return 440.0/1839.8;
                            } else {
                                return 246.0/1712.7;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                    if ( cl->stats.dump_number <= 1.5f ) {
                        return 1092.0/1658.2;
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 3679.5f ) {
                            return 504.0/2039.5;
                        } else {
                            return 660.0/1928.6;
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 9.5f ) {
                        if ( cl->stats.size_rel <= 1.11684846878f ) {
                            if ( cl->stats.dump_number <= 1.5f ) {
                                return 222.0/1672.4;
                            } else {
                                if ( cl->stats.glue <= 6.5f ) {
                                    return 167.0/2578.1;
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 6.5f ) {
                                        return 347.0/2654.8;
                                    } else {
                                        return 109.0/2126.3;
                                    }
                                }
                            }
                        } else {
                            return 259.0/1684.5;
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.223301157355f ) {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 24.5f ) {
                                return 69.0/2077.8;
                            } else {
                                return 41.0/2945.3;
                            }
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 8.5f ) {
                                return 155.0/1966.9;
                            } else {
                                if ( cl->stats.glue <= 10.5f ) {
                                    return 50.0/2638.7;
                                } else {
                                    return 88.0/2513.6;
                                }
                            }
                        }
                    }
                }
            }
        }
    } else {
        if ( cl->stats.glue <= 8.5f ) {
            if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                if ( cl->stats.rdb1_act_ranking_top_10 <= 4.5f ) {
                    if ( cl->stats.glue <= 5.5f ) {
                        if ( cl->size() <= 8.5f ) {
                            if ( cl->stats.dump_number <= 7.5f ) {
                                if ( cl->stats.glue_rel_long <= 0.476608216763f ) {
                                    return 804.0/2372.4;
                                } else {
                                    return 595.0/1341.5;
                                }
                            } else {
                                if ( cl->stats.size_rel <= 0.308034807444f ) {
                                    if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 37739.5f ) {
                                            return 514.0/1389.9;
                                        } else {
                                            return 1064.0/1511.0;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 54867.5f ) {
                                            return 494.0/1188.2;
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.124799028039f ) {
                                                return 637.0/875.5;
                                            } else {
                                                return 694.0/756.5;
                                            }
                                        }
                                    }
                                } else {
                                    return 514.0/1186.2;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 47311.5f ) {
                                if ( rdb0_last_touched_diff <= 32593.0f ) {
                                    return 491.0/1065.1;
                                } else {
                                    return 770.0/1224.5;
                                }
                            } else {
                                if ( cl->stats.size_rel <= 0.449823945761f ) {
                                    return 734.0/702.0;
                                } else {
                                    return 770.0/550.7;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 59181.5f ) {
                            if ( cl->stats.size_rel <= 0.837882101536f ) {
                                if ( cl->size() <= 12.5f ) {
                                    if ( rdb0_last_touched_diff <= 44894.5f ) {
                                        if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                                            return 638.0/1819.6;
                                        } else {
                                            if ( cl->stats.size_rel <= 0.361664712429f ) {
                                                return 540.0/1038.9;
                                            } else {
                                                return 503.0/1160.0;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.dump_number <= 8.5f ) {
                                            return 653.0/823.1;
                                        } else {
                                            return 577.0/1073.2;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 22474.0f ) {
                                        return 602.0/1083.3;
                                    } else {
                                        return 1218.0/1270.9;
                                    }
                                }
                            } else {
                                return 1091.0/944.1;
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals <= 10.5f ) {
                                if ( cl->size() <= 10.5f ) {
                                    return 917.0/1121.6;
                                } else {
                                    return 796.0/607.2;
                                }
                            } else {
                                if ( cl->size() <= 12.5f ) {
                                    if ( cl->stats.size_rel <= 0.36563462019f ) {
                                        return 756.0/597.1;
                                    } else {
                                        return 732.0/661.7;
                                    }
                                } else {
                                    if ( cl->size() <= 19.5f ) {
                                        return 1110.0/643.5;
                                    } else {
                                        return 999.0/411.5;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_last_touched_diff <= 97646.5f ) {
                        if ( cl->size() <= 10.5f ) {
                            if ( cl->stats.dump_number <= 4.5f ) {
                                return 685.0/1537.2;
                            } else {
                                if ( rdb0_last_touched_diff <= 57395.5f ) {
                                    return 582.0/1216.4;
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.490746319294f ) {
                                        return 930.0/1500.9;
                                    } else {
                                        if ( cl->stats.size_rel <= 0.32677513361f ) {
                                            return 687.0/726.2;
                                        } else {
                                            return 638.0/800.9;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.10128583014f ) {
                                return 1185.0/1565.4;
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 85.5f ) {
                                    return 1390.0/956.2;
                                } else {
                                    return 1158.0/520.5;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.antec_num_total_lits_rel <= 0.270290672779f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 23.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 260662.0f ) {
                                    if ( cl->stats.dump_number <= 25.5f ) {
                                        if ( cl->stats.glue_rel_long <= 0.37769600749f ) {
                                            return 704.0/754.5;
                                        } else {
                                            return 1605.0/1073.2;
                                        }
                                    } else {
                                        if ( cl->size() <= 7.5f ) {
                                            return 1033.0/1315.3;
                                        } else {
                                            return 738.0/689.9;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 14.5f ) {
                                        return 1000.0/635.5;
                                    } else {
                                        return 1581.0/800.9;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.110624879599f ) {
                                    return 1603.0/641.5;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 279542.5f ) {
                                        if ( cl->stats.size_rel <= 0.26968640089f ) {
                                            return 714.0/623.4;
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 0.58160674572f ) {
                                                return 812.0/516.4;
                                            } else {
                                                return 901.0/470.0;
                                            }
                                        }
                                    } else {
                                        return 1391.0/548.7;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.104070216417f ) {
                                if ( cl->stats.size_rel <= 0.608089327812f ) {
                                    return 783.0/502.3;
                                } else {
                                    return 860.0/375.2;
                                }
                            } else {
                                if ( cl->stats.size_rel <= 0.638145208359f ) {
                                    if ( rdb0_last_touched_diff <= 210830.5f ) {
                                        return 1111.0/691.9;
                                    } else {
                                        return 1054.0/488.2;
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 329947.0f ) {
                                        if ( cl->stats.rdb1_act_ranking_top_10 <= 6.5f ) {
                                            return 920.0/322.8;
                                        } else {
                                            return 1638.0/415.6;
                                        }
                                    } else {
                                        return 978.0/159.4;
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                    if ( cl->stats.glue_rel_queue <= 0.542397379875f ) {
                        if ( cl->stats.size_rel <= 0.20230537653f ) {
                            return 434.0/1700.6;
                        } else {
                            return 461.0/1387.9;
                        }
                    } else {
                        if ( cl->stats.num_overlap_literals <= 15.5f ) {
                            return 546.0/1400.0;
                        } else {
                            return 774.0/1002.6;
                        }
                    }
                } else {
                    if ( cl->stats.size_rel <= 0.299241483212f ) {
                        return 419.0/3320.5;
                    } else {
                        return 295.0/1498.9;
                    }
                }
            }
        } else {
            if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                if ( cl->stats.antecedents_glue_long_reds_var <= 0.25166541338f ) {
                    if ( cl->stats.rdb1_act_ranking_top_10 <= 4.5f ) {
                        if ( rdb0_last_touched_diff <= 65669.0f ) {
                            if ( cl->stats.glue_rel_queue <= 0.842185854912f ) {
                                if ( rdb0_last_touched_diff <= 37966.0f ) {
                                    return 794.0/1918.5;
                                } else {
                                    return 646.0/887.6;
                                }
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.187709659338f ) {
                                    return 676.0/677.8;
                                } else {
                                    return 964.0/1212.4;
                                }
                            }
                        } else {
                            if ( cl->stats.glue <= 13.5f ) {
                                return 1490.0/962.3;
                            } else {
                                return 1030.0/445.8;
                            }
                        }
                    } else {
                        if ( cl->stats.dump_number <= 15.5f ) {
                            if ( cl->stats.glue <= 33.5f ) {
                                if ( cl->stats.num_overlap_literals <= 15.5f ) {
                                    if ( cl->stats.size_rel <= 0.520814776421f ) {
                                        return 582.0/956.2;
                                    } else {
                                        return 1102.0/833.2;
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.393417268991f ) {
                                        return 896.0/482.1;
                                    } else {
                                        return 898.0/677.8;
                                    }
                                }
                            } else {
                                return 820.0/476.1;
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.889260470867f ) {
                                if ( cl->stats.glue_rel_long <= 0.611785054207f ) {
                                    return 1444.0/873.5;
                                } else {
                                    if ( rdb0_last_touched_diff <= 241479.0f ) {
                                        return 963.0/643.5;
                                    } else {
                                        return 1187.0/385.3;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 181187.0f ) {
                                    return 802.0/480.1;
                                } else {
                                    if ( cl->stats.size_rel <= 0.842469573021f ) {
                                        return 898.0/336.9;
                                    } else {
                                        return 1516.0/294.5;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.size_rel <= 0.922903776169f ) {
                        if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                            if ( cl->stats.num_overlap_literals <= 105.5f ) {
                                if ( cl->size() <= 20.5f ) {
                                    if ( cl->stats.num_overlap_literals <= 39.5f ) {
                                        if ( rdb0_last_touched_diff <= 48495.0f ) {
                                            return 628.0/857.4;
                                        } else {
                                            return 772.0/589.1;
                                        }
                                    } else {
                                        return 1048.0/653.6;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.908326983452f ) {
                                        return 923.0/651.6;
                                    } else {
                                        return 928.0/345.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.900900840759f ) {
                                    return 831.0/423.6;
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 435.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 28515.5f ) {
                                            return 997.0/270.3;
                                        } else {
                                            return 932.0/197.7;
                                        }
                                    } else {
                                        return 1066.0/141.2;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals <= 106.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 16.6770820618f ) {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 8.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 54420.5f ) {
                                            if ( cl->stats.num_antecedents_rel <= 0.264395654202f ) {
                                                return 889.0/641.5;
                                            } else {
                                                return 807.0/490.2;
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 0.72866165638f ) {
                                                if ( cl->size() <= 20.5f ) {
                                                    return 992.0/625.4;
                                                } else {
                                                    return 939.0/417.6;
                                                }
                                            } else {
                                                if ( cl->stats.rdb1_last_touched_diff <= 175605.0f ) {
                                                    if ( cl->size() <= 14.5f ) {
                                                        return 860.0/381.3;
                                                    } else {
                                                        if ( cl->stats.num_overlap_literals <= 34.5f ) {
                                                            return 881.0/353.0;
                                                        } else {
                                                            return 1061.0/330.8;
                                                        }
                                                    }
                                                } else {
                                                    if ( cl->stats.num_total_lits_antecedents <= 71.5f ) {
                                                        return 1112.0/345.0;
                                                    } else {
                                                        return 1041.0/232.0;
                                                    }
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 66.5f ) {
                                            return 1131.0/371.2;
                                        } else {
                                            return 1194.0/262.3;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 24.5f ) {
                                        return 924.0/340.9;
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 71338.0f ) {
                                            return 938.0/284.4;
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 0.998438715935f ) {
                                                return 933.0/268.3;
                                            } else {
                                                return 1199.0/127.1;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 15.9630622864f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 1104.0f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 131175.0f ) {
                                            if ( rdb0_last_touched_diff <= 81762.5f ) {
                                                return 1361.0/357.1;
                                            } else {
                                                return 1024.0/338.9;
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 0.926962137222f ) {
                                                return 901.0/290.5;
                                            } else {
                                                return 1446.0/205.8;
                                            }
                                        }
                                    } else {
                                        return 1014.0/84.7;
                                    }
                                } else {
                                    if ( cl->stats.glue <= 16.5f ) {
                                        return 1396.0/272.3;
                                    } else {
                                        if ( cl->stats.glue <= 45.5f ) {
                                            if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                                return 1532.0/76.7;
                                            } else {
                                                if ( cl->size() <= 42.5f ) {
                                                    return 1055.0/135.2;
                                                } else {
                                                    return 1029.0/159.4;
                                                }
                                            }
                                        } else {
                                            return 1216.0/223.9;
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_queue <= 0.947097837925f ) {
                            if ( cl->stats.glue_rel_long <= 0.842094540596f ) {
                                if ( rdb0_last_touched_diff <= 98781.5f ) {
                                    return 1581.0/875.5;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 256834.5f ) {
                                        if ( rdb0_last_touched_diff <= 155323.5f ) {
                                            return 944.0/264.3;
                                        } else {
                                            return 999.0/409.5;
                                        }
                                    } else {
                                        return 1038.0/215.9;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 48237.5f ) {
                                    return 887.0/342.9;
                                } else {
                                    if ( cl->stats.glue <= 13.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.501388430595f ) {
                                            return 902.0/268.3;
                                        } else {
                                            return 990.0/225.9;
                                        }
                                    } else {
                                        return 1735.0/345.0;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 27.9892158508f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.509793400764f ) {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                                        return 1162.0/506.3;
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 83.5f ) {
                                            if ( cl->stats.glue_rel_long <= 1.10377717018f ) {
                                                return 964.0/328.8;
                                            } else {
                                                return 1071.0/232.0;
                                            }
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.345363259315f ) {
                                                return 1504.0/207.8;
                                            } else {
                                                return 1144.0/211.8;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 40336.5f ) {
                                        if ( cl->stats.num_overlap_literals <= 168.5f ) {
                                            return 1366.0/455.9;
                                        } else {
                                            return 957.0/185.6;
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 510859.5f ) {
                                            if ( rdb0_last_touched_diff <= 111574.5f ) {
                                                if ( cl->stats.dump_number <= 8.5f ) {
                                                    if ( cl->stats.glue_rel_long <= 1.18154406548f ) {
                                                        return 944.0/161.4;
                                                    } else {
                                                        return 1272.0/82.7;
                                                    }
                                                } else {
                                                    return 1144.0/294.5;
                                                }
                                            } else {
                                                if ( cl->size() <= 80.5f ) {
                                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 8.5f ) {
                                                        if ( cl->stats.glue_rel_queue <= 1.06534767151f ) {
                                                            return 1033.0/246.1;
                                                        } else {
                                                            if ( cl->stats.rdb1_act_ranking_top_10 <= 6.5f ) {
                                                                return 1837.0/147.3;
                                                            } else {
                                                                return 1245.0/272.3;
                                                            }
                                                        }
                                                    } else {
                                                        return 1924.0/106.9;
                                                    }
                                                } else {
                                                    return 1089.0/46.4;
                                                }
                                            }
                                        } else {
                                            return 969.0/197.7;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 1.09528017044f ) {
                                    if ( cl->size() <= 281.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 39667.0f ) {
                                            return 1120.0/225.9;
                                        } else {
                                            if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                                if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                                    return 1002.0/115.0;
                                                } else {
                                                    return 1312.0/68.6;
                                                }
                                            } else {
                                                if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                                    return 1172.0/199.7;
                                                } else {
                                                    return 1093.0/111.0;
                                                }
                                            }
                                        }
                                    } else {
                                        return 1039.0/56.5;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 31103.0f ) {
                                        return 1171.0/157.4;
                                    } else {
                                        if ( cl->size() <= 117.5f ) {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 42.0924377441f ) {
                                                return 1011.0/113.0;
                                            } else {
                                                return 1939.0/141.2;
                                            }
                                        } else {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 132.513336182f ) {
                                                return 1164.0/18.2;
                                            } else {
                                                return 1358.0/86.7;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.rdb1_used_for_uip_creation <= 4.5f ) {
                    if ( cl->stats.num_total_lits_antecedents <= 112.5f ) {
                        return 504.0/1103.5;
                    } else {
                        return 751.0/587.0;
                    }
                } else {
                    return 415.0/1825.7;
                }
            }
        }
    }
}

static double estimator_should_keep_short_conf3_cluster0_5(
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
        if ( cl->stats.used_for_uip_creation <= 0.5f ) {
            if ( cl->stats.antecedents_glue_long_reds_var <= 9.01271629333f ) {
                if ( cl->size() <= 10.5f ) {
                    if ( rdb0_last_touched_diff <= 30763.0f ) {
                        if ( rdb0_last_touched_diff <= 15273.5f ) {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.14744053781f ) {
                                        return 313.0/1466.6;
                                    } else {
                                        return 412.0/1511.0;
                                    }
                                } else {
                                    return 839.0/2204.9;
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.567542135715f ) {
                                    if ( rdb0_last_touched_diff <= 12140.5f ) {
                                        if ( rdb0_last_touched_diff <= 11365.5f ) {
                                            return 484.0/3159.1;
                                        } else {
                                            return 224.0/1823.7;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.346964001656f ) {
                                            return 449.0/2253.3;
                                        } else {
                                            return 451.0/2902.9;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 5.5f ) {
                                        return 571.0/2564.0;
                                    } else {
                                        return 272.0/1700.6;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.size_rel <= 0.053719446063f ) {
                                return 276.0/1811.6;
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 19.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.237792924047f ) {
                                        return 516.0/1500.9;
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0181081965566f ) {
                                            return 426.0/1432.3;
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 13014.5f ) {
                                                if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                                    if ( cl->stats.num_antecedents_rel <= 0.159822016954f ) {
                                                        if ( cl->stats.dump_number <= 15.5f ) {
                                                            return 433.0/1519.0;
                                                        } else {
                                                            return 298.0/1567.5;
                                                        }
                                                    } else {
                                                        return 371.0/1962.9;
                                                    }
                                                } else {
                                                    return 279.0/1745.0;
                                                }
                                            } else {
                                                if ( cl->stats.glue_rel_queue <= 0.498457193375f ) {
                                                    return 511.0/2249.3;
                                                } else {
                                                    return 432.0/1472.6;
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 2.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.683565974236f ) {
                                            return 373.0/1289.1;
                                        } else {
                                            return 487.0/1101.5;
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0688739567995f ) {
                                            return 486.0/1188.2;
                                        } else {
                                            if ( cl->stats.dump_number <= 14.5f ) {
                                                if ( rdb0_last_touched_diff <= 21514.5f ) {
                                                    return 377.0/1450.5;
                                                } else {
                                                    return 334.0/1428.3;
                                                }
                                            } else {
                                                if ( cl->stats.num_overlap_literals <= 14.5f ) {
                                                    return 429.0/1359.7;
                                                } else {
                                                    return 415.0/1424.2;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.num_overlap_literals <= 4.5f ) {
                            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                return 455.0/1519.0;
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0298408865929f ) {
                                    return 756.0/1553.3;
                                } else {
                                    return 579.0/1575.5;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 50555.0f ) {
                                if ( cl->stats.size_rel <= 0.106395319104f ) {
                                    return 449.0/1634.0;
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 20.5f ) {
                                        if ( cl->stats.dump_number <= 12.5f ) {
                                            return 460.0/1230.6;
                                        } else {
                                            return 757.0/1759.1;
                                        }
                                    } else {
                                        if ( cl->stats.glue <= 6.5f ) {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.222631275654f ) {
                                                return 702.0/1218.5;
                                            } else {
                                                return 478.0/1244.7;
                                            }
                                        } else {
                                            return 666.0/1087.3;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.glue <= 5.5f ) {
                                    return 1030.0/1823.7;
                                } else {
                                    return 1070.0/1208.4;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                        if ( rdb0_last_touched_diff <= 46682.5f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 116.5f ) {
                                if ( cl->stats.glue_rel_queue <= 0.831869840622f ) {
                                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                        if ( cl->stats.dump_number <= 5.5f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 15141.5f ) {
                                                return 621.0/1488.8;
                                            } else {
                                                return 658.0/1170.0;
                                            }
                                        } else {
                                            if ( cl->stats.glue <= 7.5f ) {
                                                if ( cl->stats.glue_rel_long <= 0.474829733372f ) {
                                                    return 478.0/1164.0;
                                                } else {
                                                    return 464.0/1355.6;
                                                }
                                            } else {
                                                if ( cl->stats.num_antecedents_rel <= 0.188117340207f ) {
                                                    return 573.0/1406.1;
                                                } else {
                                                    return 525.0/1097.4;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.dump_number <= 4.5f ) {
                                            if ( cl->stats.num_total_lits_antecedents <= 34.5f ) {
                                                return 542.0/1053.0;
                                            } else {
                                                return 1270.0/1617.9;
                                            }
                                        } else {
                                            if ( rdb0_last_touched_diff <= 35612.5f ) {
                                                if ( cl->size() <= 24.5f ) {
                                                    if ( cl->stats.num_overlap_literals_rel <= 0.0833408609033f ) {
                                                        return 604.0/1242.7;
                                                    } else {
                                                        return 751.0/1801.5;
                                                    }
                                                } else {
                                                    return 456.0/1149.9;
                                                }
                                            } else {
                                                return 764.0/1101.5;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue <= 9.5f ) {
                                        if ( cl->stats.num_overlap_literals <= 15.5f ) {
                                            return 500.0/1063.1;
                                        } else {
                                            return 1074.0/1644.1;
                                        }
                                    } else {
                                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                            return 946.0/1484.7;
                                        } else {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.245000004768f ) {
                                                return 632.0/861.4;
                                            } else {
                                                return 1187.0/978.4;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.860347270966f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.378476142883f ) {
                                        return 706.0/839.2;
                                    } else {
                                        return 1075.0/1805.5;
                                    }
                                } else {
                                    if ( cl->stats.glue <= 10.5f ) {
                                        return 706.0/794.8;
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 2.75277781487f ) {
                                            return 794.0/653.6;
                                        } else {
                                            return 1196.0/568.9;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.size_rel <= 0.98716044426f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 92.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 94480.5f ) {
                                        if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                            return 1642.0/2394.6;
                                        } else {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.17418807745f ) {
                                                if ( cl->stats.dump_number <= 10.5f ) {
                                                    return 771.0/536.6;
                                                } else {
                                                    return 911.0/968.3;
                                                }
                                            } else {
                                                return 733.0/921.9;
                                            }
                                        }
                                    } else {
                                        return 958.0/613.3;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                        return 762.0/615.3;
                                    } else {
                                        return 1333.0/635.5;
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 74263.5f ) {
                                    return 1115.0/746.4;
                                } else {
                                    return 1022.0/332.9;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_queue <= 0.920649051666f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.18602219224f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 4072.5f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 5.5f ) {
                                        return 385.0/1759.1;
                                    } else {
                                        return 250.0/1745.0;
                                    }
                                } else {
                                    return 646.0/2245.3;
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                    if ( rdb0_last_touched_diff <= 14033.0f ) {
                                        return 692.0/2658.8;
                                    } else {
                                        return 669.0/1872.1;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.698202133179f ) {
                                        return 655.0/1888.2;
                                    } else {
                                        return 509.0/1014.7;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.dump_number <= 2.5f ) {
                                return 678.0/926.0;
                            } else {
                                return 518.0/1910.4;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.glue_rel_long <= 0.967445373535f ) {
                    if ( cl->stats.glue <= 14.5f ) {
                        if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 28681.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 12.3253440857f ) {
                                    return 576.0/1141.8;
                                } else {
                                    return 877.0/1420.2;
                                }
                            } else {
                                return 830.0/742.4;
                            }
                        } else {
                            return 1431.0/1186.2;
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 144.5f ) {
                            return 884.0/1059.1;
                        } else {
                            return 1520.0/926.0;
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 295.5f ) {
                            if ( cl->stats.num_overlap_literals <= 33.5f ) {
                                return 852.0/577.0;
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 40919.0f ) {
                                    if ( cl->stats.glue_rel_long <= 1.22775363922f ) {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 18.1825065613f ) {
                                            return 789.0/425.7;
                                        } else {
                                            return 908.0/554.8;
                                        }
                                    } else {
                                        return 1434.0/472.1;
                                    }
                                } else {
                                    return 1406.0/336.9;
                                }
                            }
                        } else {
                            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                return 1303.0/399.4;
                            } else {
                                if ( cl->stats.glue <= 26.5f ) {
                                    return 1127.0/219.9;
                                } else {
                                    if ( cl->stats.glue_rel_long <= 1.15627932549f ) {
                                        return 1050.0/157.4;
                                    } else {
                                        return 1192.0/58.5;
                                    }
                                }
                            }
                        }
                    } else {
                        return 971.0/1119.6;
                    }
                }
            }
        } else {
            if ( cl->stats.glue_rel_queue <= 0.647404551506f ) {
                if ( cl->stats.glue_rel_long <= 0.411123037338f ) {
                    if ( cl->stats.used_for_uip_creation <= 6.5f ) {
                        if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                            if ( cl->stats.size_rel <= 0.437948346138f ) {
                                if ( rdb0_last_touched_diff <= 704.5f ) {
                                    return 140.0/1866.0;
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.237187504768f ) {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 4.5f ) {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.0652383118868f ) {
                                                if ( cl->stats.num_total_lits_antecedents <= 12.5f ) {
                                                    return 293.0/2130.3;
                                                } else {
                                                    return 297.0/1502.9;
                                                }
                                            } else {
                                                if ( cl->size() <= 6.5f ) {
                                                    return 338.0/3003.8;
                                                } else {
                                                    return 274.0/1513.0;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.size_rel <= 0.166750118136f ) {
                                                return 182.0/2098.0;
                                            } else {
                                                return 118.0/1922.5;
                                            }
                                        }
                                    } else {
                                        return 373.0/2229.1;
                                    }
                                }
                            } else {
                                return 302.0/1563.4;
                            }
                        } else {
                            if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 30.5f ) {
                                    if ( cl->stats.size_rel <= 0.238660931587f ) {
                                        if ( cl->stats.glue_rel_long <= 0.309891760349f ) {
                                            return 175.0/3397.2;
                                        } else {
                                            return 138.0/1815.6;
                                        }
                                    } else {
                                        return 75.0/1954.8;
                                    }
                                } else {
                                    return 193.0/2253.3;
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 4938.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.0685006827116f ) {
                                        return 215.0/1626.0;
                                    } else {
                                        if ( cl->stats.dump_number <= 27.5f ) {
                                            if ( rdb0_last_touched_diff <= 1734.5f ) {
                                                return 125.0/1872.1;
                                            } else {
                                                return 162.0/1833.7;
                                            }
                                        } else {
                                            return 114.0/2079.9;
                                        }
                                    }
                                } else {
                                    return 334.0/2303.8;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 15.5f ) {
                            if ( cl->stats.size_rel <= 0.205334037542f ) {
                                if ( rdb0_last_touched_diff <= 4042.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 6381.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 2197.5f ) {
                                            if ( rdb0_last_touched_diff <= 1408.5f ) {
                                                if ( cl->stats.used_for_uip_creation <= 75.5f ) {
                                                    if ( cl->stats.num_antecedents_rel <= 0.0701193362474f ) {
                                                        return 48.0/3814.8;
                                                    } else {
                                                        return 46.0/2582.2;
                                                    }
                                                } else {
                                                    return 49.0/2148.4;
                                                }
                                            } else {
                                                return 86.0/2753.6;
                                            }
                                        } else {
                                            return 109.0/3102.6;
                                        }
                                    } else {
                                        return 115.0/1847.9;
                                    }
                                } else {
                                    return 182.0/2263.4;
                                }
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 12.5f ) {
                                    return 59.0/2332.0;
                                } else {
                                    return 40.0/3867.2;
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 2620.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.323574662209f ) {
                                    if ( cl->stats.glue_rel_long <= 0.151693418622f ) {
                                        return 105.0/2217.0;
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0110916970298f ) {
                                            return 65.0/3314.5;
                                        } else {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 4.5f ) {
                                                return 171.0/3122.8;
                                            } else {
                                                if ( cl->stats.num_overlap_literals_rel <= 0.0429457128048f ) {
                                                    if ( cl->stats.num_antecedents_rel <= 0.100424274802f ) {
                                                        return 101.0/2969.5;
                                                    } else {
                                                        return 57.0/2009.3;
                                                    }
                                                } else {
                                                    if ( cl->stats.rdb1_last_touched_diff <= 1022.5f ) {
                                                        return 43.0/2907.0;
                                                    } else {
                                                        return 56.0/2089.9;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    return 66.0/3786.5;
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 3513.0f ) {
                                    if ( rdb0_last_touched_diff <= 4995.5f ) {
                                        return 132.0/2761.7;
                                    } else {
                                        return 145.0/1801.5;
                                    }
                                } else {
                                    return 203.0/1914.4;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_last_touched_diff <= 8018.0f ) {
                        if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                return 473.0/2332.0;
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 15.5f ) {
                                    return 183.0/2596.3;
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0525937974453f ) {
                                        return 240.0/1636.0;
                                    } else {
                                        if ( cl->size() <= 11.5f ) {
                                            return 243.0/2606.4;
                                        } else {
                                            return 288.0/1894.3;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 1.57831788063f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 2388.0f ) {
                                        if ( rdb0_last_touched_diff <= 1719.0f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.177080899477f ) {
                                                if ( cl->stats.num_antecedents_rel <= 0.098962366581f ) {
                                                    if ( rdb0_last_touched_diff <= 526.5f ) {
                                                        return 67.0/2838.4;
                                                    } else {
                                                        return 86.0/2158.5;
                                                    }
                                                } else {
                                                    if ( cl->stats.rdb1_used_for_uip_creation <= 20.5f ) {
                                                        if ( rdb0_last_touched_diff <= 533.5f ) {
                                                            return 48.0/2027.4;
                                                        } else {
                                                            return 64.0/2075.8;
                                                        }
                                                    } else {
                                                        return 25.0/3092.6;
                                                    }
                                                }
                                            } else {
                                                return 24.0/2138.4;
                                            }
                                        } else {
                                            if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                                if ( cl->stats.glue_rel_long <= 0.502687811852f ) {
                                                    return 136.0/2578.1;
                                                } else {
                                                    return 110.0/2804.1;
                                                }
                                            } else {
                                                if ( cl->stats.num_total_lits_antecedents <= 25.5f ) {
                                                    return 136.0/2697.2;
                                                } else {
                                                    return 151.0/1876.1;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.589755117893f ) {
                                            if ( cl->stats.glue_rel_queue <= 0.525187134743f ) {
                                                if ( cl->stats.num_overlap_literals_rel <= 0.0424376092851f ) {
                                                    return 133.0/2039.5;
                                                } else {
                                                    return 163.0/3552.5;
                                                }
                                            } else {
                                                return 200.0/2650.8;
                                            }
                                        } else {
                                            return 87.0/1966.9;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.225446045399f ) {
                                        return 151.0/3056.2;
                                    } else {
                                        return 152.0/2174.7;
                                    }
                                }
                            } else {
                                return 165.0/2505.5;
                            }
                        }
                    } else {
                        if ( cl->size() <= 11.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 16490.0f ) {
                                if ( cl->stats.size_rel <= 0.331693947315f ) {
                                    return 369.0/3364.9;
                                } else {
                                    return 199.0/2558.0;
                                }
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                    return 435.0/2632.6;
                                } else {
                                    return 197.0/1751.0;
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 2184.5f ) {
                                return 256.0/2447.0;
                            } else {
                                if ( rdb0_last_touched_diff <= 5365.0f ) {
                                    return 296.0/1662.3;
                                } else {
                                    return 329.0/1430.3;
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                    if ( rdb0_last_touched_diff <= 2503.5f ) {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.232889980078f ) {
                            if ( rdb0_last_touched_diff <= 962.5f ) {
                                return 195.0/2862.6;
                            } else {
                                return 277.0/2305.8;
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.998572528362f ) {
                                if ( rdb0_last_touched_diff <= 735.5f ) {
                                    return 197.0/2081.9;
                                } else {
                                    return 392.0/2888.8;
                                }
                            } else {
                                return 263.0/1710.7;
                            }
                        }
                    } else {
                        if ( cl->stats.glue <= 7.5f ) {
                            if ( cl->size() <= 10.5f ) {
                                if ( cl->stats.glue_rel_queue <= 0.797569453716f ) {
                                    return 258.0/2037.5;
                                } else {
                                    return 244.0/1577.5;
                                }
                            } else {
                                return 428.0/1849.9;
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 210.5f ) {
                                if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.989127397537f ) {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 1.01999998093f ) {
                                            return 333.0/1506.9;
                                        } else {
                                            return 394.0/1448.4;
                                        }
                                    } else {
                                        return 420.0/1216.4;
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 9.5f ) {
                                        return 317.0/1628.0;
                                    } else {
                                        return 232.0/1682.4;
                                    }
                                }
                            } else {
                                return 462.0/1117.6;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_overlap_literals <= 25.5f ) {
                        if ( cl->stats.num_overlap_literals_rel <= 0.0317842587829f ) {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 12.5f ) {
                                return 345.0/3264.0;
                            } else {
                                return 134.0/3722.0;
                            }
                        } else {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 12.5f ) {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                                    return 173.0/1981.0;
                                } else {
                                    if ( rdb0_last_touched_diff <= 2558.5f ) {
                                        if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                            return 127.0/1987.1;
                                        } else {
                                            return 48.0/2305.8;
                                        }
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 29.5f ) {
                                            return 130.0/1890.2;
                                        } else {
                                            return 157.0/1821.6;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 18.5f ) {
                                    return 133.0/3340.7;
                                } else {
                                    return 43.0/3397.2;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.dump_number <= 1.5f ) {
                            return 268.0/2043.5;
                        } else {
                            if ( rdb0_last_touched_diff <= 2130.5f ) {
                                if ( cl->stats.glue_rel_queue <= 0.839199542999f ) {
                                    return 77.0/2707.2;
                                } else {
                                    if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                        return 119.0/2116.2;
                                    } else {
                                        return 74.0/2200.9;
                                    }
                                }
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 6.5f ) {
                                    return 362.0/3072.4;
                                } else {
                                    return 138.0/2041.5;
                                }
                            }
                        }
                    }
                }
            }
        }
    } else {
        if ( cl->stats.glue_rel_queue <= 0.875937998295f ) {
            if ( cl->size() <= 9.5f ) {
                if ( cl->stats.rdb1_last_touched_diff <= 48537.5f ) {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                        if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                            if ( cl->stats.num_overlap_literals <= 23.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 19954.0f ) {
                                    return 729.0/2334.0;
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.443908393383f ) {
                                        if ( cl->stats.size_rel <= 0.142522037029f ) {
                                            return 413.0/1164.0;
                                        } else {
                                            return 701.0/1488.8;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                                            return 521.0/1164.0;
                                        } else {
                                            return 635.0/960.2;
                                        }
                                    }
                                }
                            } else {
                                return 581.0/1109.5;
                            }
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                return 492.0/2362.3;
                            } else {
                                return 333.0/3490.0;
                            }
                        }
                    } else {
                        return 206.0/1908.4;
                    }
                } else {
                    if ( cl->stats.num_total_lits_antecedents <= 23.5f ) {
                        if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 147807.5f ) {
                                if ( rdb0_last_touched_diff <= 111712.5f ) {
                                    if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                        return 680.0/1196.3;
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.14849974215f ) {
                                            return 1049.0/1595.7;
                                        } else {
                                            return 1033.0/1297.1;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0883083045483f ) {
                                        return 862.0/881.6;
                                    } else {
                                        return 805.0/944.1;
                                    }
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 7.5f ) {
                                    if ( rdb0_last_touched_diff <= 270438.0f ) {
                                        return 1085.0/1113.6;
                                    } else {
                                        return 1119.0/825.1;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 292627.5f ) {
                                        return 1176.0/994.5;
                                    } else {
                                        return 837.0/455.9;
                                    }
                                }
                            }
                        } else {
                            return 533.0/2664.9;
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 178633.5f ) {
                            if ( rdb0_last_touched_diff <= 69149.5f ) {
                                return 516.0/1063.1;
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                    return 1105.0/1285.0;
                                } else {
                                    return 1107.0/990.5;
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 297664.5f ) {
                                return 876.0/778.7;
                            } else {
                                return 870.0/417.6;
                            }
                        }
                    }
                }
            } else {
                if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                    if ( cl->stats.glue_rel_long <= 0.466270148754f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 54281.0f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 82.5f ) {
                                if ( cl->stats.num_overlap_literals <= 11.5f ) {
                                    return 670.0/1759.1;
                                } else {
                                    return 567.0/982.4;
                                }
                            } else {
                                return 719.0/1002.6;
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 163426.5f ) {
                                if ( cl->stats.num_overlap_literals <= 9.5f ) {
                                    return 1002.0/1020.8;
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.197176367044f ) {
                                        return 1289.0/877.5;
                                    } else {
                                        return 729.0/694.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.377216219902f ) {
                                    return 1626.0/891.7;
                                } else {
                                    return 1651.0/613.3;
                                }
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 122700.0f ) {
                            if ( cl->stats.size_rel <= 0.772798061371f ) {
                                if ( cl->stats.glue_rel_long <= 0.853214144707f ) {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 4.5f ) {
                                        if ( rdb0_last_touched_diff <= 70664.0f ) {
                                            if ( cl->stats.dump_number <= 6.5f ) {
                                                if ( cl->stats.glue_rel_queue <= 0.639427006245f ) {
                                                    return 747.0/1089.4;
                                                } else {
                                                    return 1111.0/1121.6;
                                                }
                                            } else {
                                                if ( rdb0_last_touched_diff <= 43992.5f ) {
                                                    return 515.0/1101.5;
                                                } else {
                                                    return 686.0/982.4;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 0.647957205772f ) {
                                                return 739.0/585.0;
                                            } else {
                                                return 864.0/589.1;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.0996875017881f ) {
                                            return 1262.0/1617.9;
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 0.62543463707f ) {
                                                return 750.0/593.1;
                                            } else {
                                                return 1551.0/784.7;
                                            }
                                        }
                                    }
                                } else {
                                    return 1229.0/760.5;
                                }
                            } else {
                                if ( cl->size() <= 100.5f ) {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 1.4561111927f ) {
                                            return 692.0/724.2;
                                        } else {
                                            return 882.0/629.4;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 55353.5f ) {
                                            return 1212.0/921.9;
                                        } else {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.511150658131f ) {
                                                return 1111.0/568.9;
                                            } else {
                                                return 953.0/435.7;
                                            }
                                        }
                                    }
                                } else {
                                    return 1143.0/326.8;
                                }
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals <= 16.5f ) {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 4.5f ) {
                                    return 1153.0/714.1;
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.270408034325f ) {
                                        if ( cl->stats.dump_number <= 63.5f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 163101.5f ) {
                                                return 951.0/566.9;
                                            } else {
                                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.245000004768f ) {
                                                    return 1203.0/548.7;
                                                } else {
                                                    return 1089.0/347.0;
                                                }
                                            }
                                        } else {
                                            return 831.0/443.8;
                                        }
                                    } else {
                                        return 1648.0/508.4;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 5.5f ) {
                                    if ( cl->size() <= 31.5f ) {
                                        if ( cl->stats.glue_rel_long <= 0.721807122231f ) {
                                            return 1534.0/831.1;
                                        } else {
                                            return 1323.0/486.2;
                                        }
                                    } else {
                                        return 1432.0/411.5;
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 72.5f ) {
                                        if ( cl->stats.glue_rel_long <= 0.716587901115f ) {
                                            if ( cl->size() <= 15.5f ) {
                                                return 895.0/427.7;
                                            } else {
                                                if ( cl->stats.antecedents_glue_long_reds_var <= 1.08364892006f ) {
                                                    return 889.0/304.6;
                                                } else {
                                                    return 1638.0/395.4;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.676082253456f ) {
                                                if ( cl->stats.glue_rel_long <= 0.828888058662f ) {
                                                    return 1483.0/383.3;
                                                } else {
                                                    return 1073.0/159.4;
                                                }
                                            } else {
                                                return 1240.0/383.3;
                                            }
                                        }
                                    } else {
                                        return 873.0/383.3;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                        if ( cl->stats.num_overlap_literals_rel <= 0.06222230196f ) {
                            return 485.0/3130.9;
                        } else {
                            if ( cl->size() <= 22.5f ) {
                                if ( cl->stats.size_rel <= 0.561320781708f ) {
                                    return 398.0/1615.9;
                                } else {
                                    return 298.0/1595.7;
                                }
                            } else {
                                return 397.0/2045.6;
                            }
                        }
                    } else {
                        return 703.0/1972.9;
                    }
                }
            }
        } else {
            if ( rdb0_last_touched_diff <= 9940.5f ) {
                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                    if ( cl->size() <= 42.5f ) {
                        return 641.0/2610.4;
                    } else {
                        return 295.0/1531.1;
                    }
                } else {
                    return 671.0/1500.9;
                }
            } else {
                if ( cl->stats.num_overlap_literals <= 32.5f ) {
                    if ( cl->stats.size_rel <= 0.69755589962f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 27.5f ) {
                            if ( cl->stats.dump_number <= 11.5f ) {
                                return 732.0/1111.5;
                            } else {
                                return 1171.0/974.4;
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 91742.5f ) {
                                if ( rdb0_last_touched_diff <= 45655.5f ) {
                                    return 697.0/913.8;
                                } else {
                                    return 1224.0/917.9;
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 170907.0f ) {
                                    return 866.0/470.0;
                                } else {
                                    return 1473.0/520.5;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.num_overlap_literals <= 11.5f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.173749998212f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 111031.5f ) {
                                    return 810.0/845.3;
                                } else {
                                    return 956.0/290.5;
                                }
                            } else {
                                return 1411.0/451.9;
                            }
                        } else {
                            if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.0413223132491f ) {
                                    return 981.0/702.0;
                                } else {
                                    if ( cl->stats.glue <= 13.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 66285.0f ) {
                                            return 896.0/419.6;
                                        } else {
                                            if ( cl->stats.dump_number <= 20.5f ) {
                                                return 936.0/207.8;
                                            } else {
                                                return 941.0/304.6;
                                            }
                                        }
                                    } else {
                                        return 1784.0/421.6;
                                    }
                                }
                            } else {
                                return 1648.0/363.1;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_total_lits_antecedents <= 212.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 71304.5f ) {
                            if ( cl->size() <= 19.5f ) {
                                if ( cl->stats.glue <= 11.5f ) {
                                    if ( rdb0_last_touched_diff <= 47931.0f ) {
                                        return 707.0/665.7;
                                    } else {
                                        return 990.0/574.9;
                                    }
                                } else {
                                    return 1103.0/336.9;
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 1.07917952538f ) {
                                    if ( cl->size() <= 29.5f ) {
                                        return 974.0/316.7;
                                    } else {
                                        return 1002.0/512.4;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                                        return 972.0/284.4;
                                    } else {
                                        if ( cl->stats.size_rel <= 1.16340398788f ) {
                                            return 1150.0/355.0;
                                        } else {
                                            return 1312.0/169.5;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->size() <= 15.5f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.926130652428f ) {
                                    return 1581.0/645.5;
                                } else {
                                    return 1206.0/238.0;
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 1.13449037075f ) {
                                    if ( cl->stats.glue_rel_long <= 0.908337473869f ) {
                                        return 1660.0/484.2;
                                    } else {
                                        if ( cl->stats.glue <= 11.5f ) {
                                            return 1701.0/476.1;
                                        } else {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.346470296383f ) {
                                                return 1002.0/205.8;
                                            } else {
                                                if ( cl->stats.glue <= 16.5f ) {
                                                    if ( cl->stats.antecedents_glue_long_reds_var <= 9.41874980927f ) {
                                                        return 1073.0/153.3;
                                                    } else {
                                                        return 989.0/189.6;
                                                    }
                                                } else {
                                                    return 990.0/111.0;
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 0.829462409019f ) {
                                        return 1592.0/304.6;
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 1.36883044243f ) {
                                            if ( cl->stats.glue_rel_queue <= 1.27643597126f ) {
                                                if ( rdb0_last_touched_diff <= 179412.0f ) {
                                                    return 1242.0/189.6;
                                                } else {
                                                    return 1564.0/137.2;
                                                }
                                            } else {
                                                return 957.0/191.6;
                                            }
                                        } else {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 13.0229587555f ) {
                                                return 990.0/119.0;
                                            } else {
                                                return 1347.0/80.7;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 721.5f ) {
                            if ( cl->stats.glue_rel_long <= 1.24033868313f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 41340.0f ) {
                                    if ( cl->stats.glue_rel_long <= 1.05708360672f ) {
                                        return 944.0/280.4;
                                    } else {
                                        return 988.0/234.0;
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 28.5f ) {
                                        if ( rdb0_last_touched_diff <= 210310.0f ) {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 40.8228988647f ) {
                                                if ( cl->stats.glue_rel_long <= 0.929348647594f ) {
                                                    return 927.0/225.9;
                                                } else {
                                                    if ( cl->stats.glue_rel_long <= 1.06607031822f ) {
                                                        return 1355.0/197.7;
                                                    } else {
                                                        return 1514.0/304.6;
                                                    }
                                                }
                                            } else {
                                                return 1905.0/213.8;
                                            }
                                        } else {
                                            return 1047.0/96.8;
                                        }
                                    } else {
                                        return 1633.0/397.4;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 35854.5f ) {
                                    return 1487.0/298.6;
                                } else {
                                    if ( cl->stats.glue <= 21.5f ) {
                                        if ( cl->stats.dump_number <= 21.5f ) {
                                            return 1853.0/175.5;
                                        } else {
                                            return 974.0/189.6;
                                        }
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.670946955681f ) {
                                            return 1610.0/195.7;
                                        } else {
                                            if ( rdb0_last_touched_diff <= 170331.0f ) {
                                                return 1364.0/100.9;
                                            } else {
                                                return 1040.0/32.3;
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 1.02919483185f ) {
                                return 1906.0/278.4;
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 91586.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 1151.5f ) {
                                        return 1397.0/240.1;
                                    } else {
                                        if ( cl->size() <= 224.5f ) {
                                            return 1407.0/78.7;
                                        } else {
                                            return 1242.0/115.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 19.5f ) {
                                        if ( cl->stats.glue <= 40.5f ) {
                                            return 1066.0/40.3;
                                        } else {
                                            return 1039.0/28.2;
                                        }
                                    } else {
                                        return 1503.0/133.1;
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

static double estimator_should_keep_short_conf3_cluster0_6(
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
    if ( cl->stats.used_for_uip_creation <= 0.5f ) {
        if ( cl->size() <= 12.5f ) {
            if ( cl->stats.glue <= 7.5f ) {
                if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 17.5f ) {
                            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 20761.5f ) {
                                    if ( cl->stats.glue <= 3.5f ) {
                                        return 332.0/1579.6;
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 4.5f ) {
                                            return 531.0/2326.0;
                                        } else {
                                            return 420.0/1464.6;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 0.156560093164f ) {
                                        return 388.0/1264.9;
                                    } else {
                                        return 432.0/1333.4;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0489829778671f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0119302943349f ) {
                                        return 597.0/1143.8;
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 23311.5f ) {
                                            if ( cl->stats.num_antecedents_rel <= 0.10615709424f ) {
                                                return 506.0/2015.3;
                                            } else {
                                                return 466.0/1440.4;
                                            }
                                        } else {
                                            if ( rdb0_last_touched_diff <= 47562.0f ) {
                                                return 466.0/1065.1;
                                            } else {
                                                return 682.0/1202.3;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 4.5f ) {
                                        return 352.0/1543.3;
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.438170522451f ) {
                                            return 396.0/1500.9;
                                        } else {
                                            return 524.0/1613.9;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->size() <= 8.5f ) {
                                if ( rdb0_last_touched_diff <= 42240.5f ) {
                                    if ( rdb0_last_touched_diff <= 28399.0f ) {
                                        if ( cl->stats.glue <= 4.5f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.118932560086f ) {
                                                return 374.0/1381.9;
                                            } else {
                                                return 311.0/1492.8;
                                            }
                                        } else {
                                            if ( rdb0_last_touched_diff <= 22003.0f ) {
                                                return 737.0/2231.2;
                                            } else {
                                                return 355.0/1492.8;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.129314750433f ) {
                                            return 491.0/1361.7;
                                        } else {
                                            return 415.0/1323.4;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 58730.5f ) {
                                        return 852.0/1914.4;
                                    } else {
                                        return 604.0/1002.6;
                                    }
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                    if ( rdb0_last_touched_diff <= 27570.5f ) {
                                        return 714.0/2196.9;
                                    } else {
                                        return 704.0/1371.8;
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 48054.0f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 26.5f ) {
                                            return 716.0/1613.9;
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 14749.0f ) {
                                                return 438.0/1232.6;
                                            } else {
                                                return 562.0/1307.2;
                                            }
                                        }
                                    } else {
                                        return 1039.0/1008.7;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.size_rel <= 0.0814118310809f ) {
                            return 385.0/2840.4;
                        } else {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.0766925513744f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 13.5f ) {
                                    if ( cl->stats.size_rel <= 0.146229654551f ) {
                                        return 330.0/1436.3;
                                    } else {
                                        return 339.0/1807.5;
                                    }
                                } else {
                                    return 552.0/1991.1;
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 13516.0f ) {
                                    if ( cl->size() <= 7.5f ) {
                                        if ( cl->stats.size_rel <= 0.276593983173f ) {
                                            return 274.0/2015.3;
                                        } else {
                                            return 179.0/1676.4;
                                        }
                                    } else {
                                        return 502.0/3132.9;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 8.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 6038.0f ) {
                                            return 262.0/1537.2;
                                        } else {
                                            return 313.0/1577.5;
                                        }
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.262215733528f ) {
                                            return 328.0/1444.4;
                                        } else {
                                            return 380.0/1517.0;
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_act_ranking_top_10 <= 5.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 49005.0f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 18695.5f ) {
                                if ( cl->stats.num_overlap_literals <= 9.5f ) {
                                    return 527.0/1906.4;
                                } else {
                                    return 493.0/1230.6;
                                }
                            } else {
                                if ( cl->stats.size_rel <= 0.147548735142f ) {
                                    return 631.0/1876.1;
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.108094148338f ) {
                                        return 814.0/1279.0;
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.279141902924f ) {
                                            if ( cl->stats.num_overlap_literals <= 9.5f ) {
                                                return 592.0/1333.4;
                                            } else {
                                                return 488.0/1083.3;
                                            }
                                        } else {
                                            return 663.0/1055.1;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 180750.0f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.148164376616f ) {
                                    if ( rdb0_last_touched_diff <= 103960.0f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 14.5f ) {
                                            return 621.0/1087.3;
                                        } else {
                                            return 767.0/1135.8;
                                        }
                                    } else {
                                        return 1055.0/1270.9;
                                    }
                                } else {
                                    if ( cl->size() <= 8.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0863222256303f ) {
                                            return 786.0/798.9;
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 25.5f ) {
                                                return 606.0/877.5;
                                            } else {
                                                return 761.0/970.3;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.270230770111f ) {
                                            return 682.0/685.9;
                                        } else {
                                            return 857.0/700.0;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.49075204134f ) {
                                    return 905.0/687.9;
                                } else {
                                    return 759.0/526.5;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 117831.0f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.0355029590428f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.151804298162f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0734474807978f ) {
                                        return 691.0/1055.1;
                                    } else {
                                        return 598.0/1016.7;
                                    }
                                } else {
                                    return 719.0/942.1;
                                }
                            } else {
                                return 903.0/930.0;
                            }
                        } else {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.148045033216f ) {
                                if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                    return 851.0/714.1;
                                } else {
                                    if ( rdb0_last_touched_diff <= 273381.0f ) {
                                        if ( rdb0_last_touched_diff <= 178238.5f ) {
                                            return 885.0/857.4;
                                        } else {
                                            return 1083.0/804.9;
                                        }
                                    } else {
                                        return 1569.0/825.1;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 277563.5f ) {
                                    if ( cl->stats.dump_number <= 25.5f ) {
                                        return 1146.0/691.9;
                                    } else {
                                        return 1011.0/855.3;
                                    }
                                } else {
                                    return 1377.0/524.5;
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.rdb1_last_touched_diff <= 38550.5f ) {
                    if ( cl->stats.num_overlap_literals <= 51.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 16832.5f ) {
                            if ( cl->stats.dump_number <= 2.5f ) {
                                return 666.0/1539.2;
                            } else {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                    return 615.0/1987.1;
                                } else {
                                    return 325.0/1636.0;
                                }
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 26.5f ) {
                                return 638.0/1494.8;
                            } else {
                                return 839.0/1147.9;
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                            return 715.0/1034.9;
                        } else {
                            return 938.0/671.8;
                        }
                    }
                } else {
                    if ( cl->stats.num_total_lits_antecedents <= 51.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 116049.5f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 26.5f ) {
                                return 1155.0/1525.1;
                            } else {
                                return 1053.0/982.4;
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 240682.0f ) {
                                return 1257.0/897.7;
                            } else {
                                return 1123.0/534.6;
                            }
                        }
                    } else {
                        if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                            return 911.0/488.2;
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.833524703979f ) {
                                return 815.0/429.7;
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 120363.5f ) {
                                    return 924.0/272.3;
                                } else {
                                    return 1020.0/199.7;
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if ( rdb0_last_touched_diff <= 52559.5f ) {
                if ( cl->stats.num_total_lits_antecedents <= 145.5f ) {
                    if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                        if ( cl->stats.glue_rel_long <= 0.892991423607f ) {
                            if ( cl->stats.dump_number <= 4.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 16088.0f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.63251131773f ) {
                                        if ( cl->stats.glue_rel_long <= 0.509182572365f ) {
                                            return 603.0/1646.1;
                                        } else {
                                            return 462.0/1147.9;
                                        }
                                    } else {
                                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                            return 848.0/2019.3;
                                        } else {
                                            return 684.0/990.5;
                                        }
                                    }
                                } else {
                                    return 1139.0/1387.9;
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 28490.0f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                        if ( cl->stats.glue_rel_long <= 0.50739556551f ) {
                                            return 658.0/1914.4;
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 40.5f ) {
                                                return 548.0/1603.8;
                                            } else {
                                                if ( cl->stats.antecedents_glue_long_reds_var <= 1.55328798294f ) {
                                                    return 488.0/1164.0;
                                                } else {
                                                    return 512.0/1057.1;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 14219.5f ) {
                                            return 492.0/2555.9;
                                        } else {
                                            return 374.0/1444.4;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 24.5f ) {
                                        return 841.0/1658.2;
                                    } else {
                                        return 642.0/990.5;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                if ( cl->stats.dump_number <= 5.5f ) {
                                    if ( cl->stats.size_rel <= 0.925487041473f ) {
                                        return 1323.0/1236.6;
                                    } else {
                                        return 1392.0/663.7;
                                    }
                                } else {
                                    return 944.0/2049.6;
                                }
                            } else {
                                return 542.0/1797.4;
                            }
                        }
                    } else {
                        if ( cl->stats.dump_number <= 5.5f ) {
                            if ( cl->stats.num_overlap_literals_rel <= 0.474095851183f ) {
                                if ( cl->stats.glue_rel_long <= 0.865761339664f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                                        if ( cl->stats.num_overlap_literals <= 11.5f ) {
                                            return 979.0/1632.0;
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 0.561236262321f ) {
                                                return 617.0/845.3;
                                            } else {
                                                if ( cl->stats.rdb1_last_touched_diff <= 25273.5f ) {
                                                    return 754.0/748.4;
                                                } else {
                                                    return 791.0/504.3;
                                                }
                                            }
                                        }
                                    } else {
                                        return 612.0/1127.7;
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 26355.5f ) {
                                        return 1253.0/1038.9;
                                    } else {
                                        if ( cl->stats.glue <= 12.5f ) {
                                            return 1045.0/696.0;
                                        } else {
                                            return 1428.0/583.0;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.glue <= 10.5f ) {
                                    return 1057.0/970.3;
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.781752228737f ) {
                                        return 829.0/361.1;
                                    } else {
                                        return 1091.0/284.4;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 18750.0f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.0493827164173f ) {
                                    return 589.0/1837.8;
                                } else {
                                    return 847.0/1533.2;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 11.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.13911563158f ) {
                                        return 530.0/1028.8;
                                    } else {
                                        return 528.0/1000.6;
                                    }
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.536911010742f ) {
                                        return 763.0/1392.0;
                                    } else {
                                        if ( cl->stats.dump_number <= 14.5f ) {
                                            return 711.0/734.3;
                                        } else {
                                            return 716.0/873.5;
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                        if ( cl->stats.dump_number <= 5.5f ) {
                            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.glue_rel_long <= 1.03611326218f ) {
                                    if ( rdb0_last_touched_diff <= 23424.0f ) {
                                        return 694.0/778.7;
                                    } else {
                                        return 776.0/607.2;
                                    }
                                } else {
                                    return 1565.0/484.2;
                                }
                            } else {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 13.2099380493f ) {
                                    if ( cl->stats.size_rel <= 0.945022106171f ) {
                                        if ( cl->stats.glue_rel_long <= 0.851142585278f ) {
                                            return 993.0/748.4;
                                        } else {
                                            if ( cl->stats.num_overlap_literals <= 215.5f ) {
                                                return 913.0/347.0;
                                            } else {
                                                return 973.0/230.0;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 1.00026321411f ) {
                                            return 1140.0/550.7;
                                        } else {
                                            return 1826.0/377.2;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue <= 15.5f ) {
                                        return 1122.0/338.9;
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.493874490261f ) {
                                            return 1144.0/209.8;
                                        } else {
                                            if ( cl->stats.num_overlap_literals <= 290.5f ) {
                                                if ( cl->stats.glue_rel_long <= 1.3692510128f ) {
                                                    return 1650.0/300.6;
                                                } else {
                                                    return 1028.0/88.8;
                                                }
                                            } else {
                                                if ( cl->stats.glue_rel_long <= 1.31634414196f ) {
                                                    if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                                        return 1174.0/88.8;
                                                    } else {
                                                        return 1121.0/181.6;
                                                    }
                                                } else {
                                                    return 1572.0/50.4;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                return 859.0/1406.1;
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.650210976601f ) {
                                    return 1046.0/972.3;
                                } else {
                                    return 944.0/1208.4;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 5.5f ) {
                            return 694.0/1111.5;
                        } else {
                            return 376.0/1333.4;
                        }
                    }
                }
            } else {
                if ( cl->stats.num_overlap_literals <= 23.5f ) {
                    if ( cl->stats.rdb1_last_touched_diff <= 119592.5f ) {
                        if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                            return 1132.0/1337.5;
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.88823390007f ) {
                                if ( cl->stats.dump_number <= 11.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.608761787415f ) {
                                        return 1260.0/1155.9;
                                    } else {
                                        if ( cl->size() <= 18.5f ) {
                                            return 869.0/579.0;
                                        } else {
                                            return 1078.0/554.8;
                                        }
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 95131.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.562776386738f ) {
                                            return 603.0/893.7;
                                        } else {
                                            return 895.0/994.5;
                                        }
                                    } else {
                                        return 1245.0/1131.7;
                                    }
                                }
                            } else {
                                if ( cl->stats.dump_number <= 12.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.173609375954f ) {
                                        return 1264.0/389.3;
                                    } else {
                                        return 1342.0/574.9;
                                    }
                                } else {
                                    return 756.0/730.3;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 168254.0f ) {
                            if ( cl->stats.glue_rel_queue <= 0.778072476387f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.152766570449f ) {
                                    return 792.0/532.6;
                                } else {
                                    return 1052.0/577.0;
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 7.5f ) {
                                    return 900.0/322.8;
                                } else {
                                    return 952.0/338.9;
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 271720.0f ) {
                                if ( cl->stats.glue_rel_long <= 0.647082448006f ) {
                                    return 1338.0/665.7;
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.260835945606f ) {
                                        return 1262.0/486.2;
                                    } else {
                                        return 1613.0/338.9;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.175671428442f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 622629.0f ) {
                                        if ( cl->stats.glue <= 14.5f ) {
                                            if ( cl->stats.glue_rel_long <= 0.698973059654f ) {
                                                return 1296.0/470.0;
                                            } else {
                                                return 1102.0/322.8;
                                            }
                                        } else {
                                            return 1030.0/230.0;
                                        }
                                    } else {
                                        return 1018.0/451.9;
                                    }
                                } else {
                                    return 1515.0/280.4;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                        if ( cl->stats.glue <= 13.5f ) {
                            if ( cl->stats.glue <= 8.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 61156.5f ) {
                                    return 681.0/726.2;
                                } else {
                                    return 1267.0/897.7;
                                }
                            } else {
                                if ( cl->stats.dump_number <= 14.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.319697797298f ) {
                                        return 813.0/508.4;
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.899093389511f ) {
                                            return 1526.0/694.0;
                                        } else {
                                            return 936.0/314.7;
                                        }
                                    }
                                } else {
                                    return 1205.0/930.0;
                                }
                            }
                        } else {
                            if ( cl->stats.num_antecedents_rel <= 0.510736048222f ) {
                                if ( cl->stats.glue_rel_queue <= 1.02239990234f ) {
                                    return 1404.0/714.1;
                                } else {
                                    return 1420.0/278.4;
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 528.5f ) {
                                    if ( cl->stats.dump_number <= 10.5f ) {
                                        return 1675.0/256.2;
                                    } else {
                                        return 942.0/342.9;
                                    }
                                } else {
                                    return 1862.0/165.4;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_queue <= 0.945444703102f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 93743.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.494897961617f ) {
                                    return 738.0/683.9;
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.813151836395f ) {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 3.55777788162f ) {
                                            return 883.0/328.8;
                                        } else {
                                            return 1036.0/585.0;
                                        }
                                    } else {
                                        return 1646.0/550.7;
                                    }
                                }
                            } else {
                                if ( cl->stats.dump_number <= 12.5f ) {
                                    return 1426.0/292.5;
                                } else {
                                    if ( rdb0_last_touched_diff <= 134248.0f ) {
                                        return 799.0/476.1;
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.533605098724f ) {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 1.25765299797f ) {
                                                return 924.0/490.2;
                                            } else {
                                                return 934.0/369.2;
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_act_ranking_top_10 <= 4.5f ) {
                                                if ( cl->stats.glue_rel_queue <= 0.803161859512f ) {
                                                    return 815.0/359.1;
                                                } else {
                                                    return 913.0/349.0;
                                                }
                                            } else {
                                                if ( cl->stats.rdb1_act_ranking_top_10 <= 8.5f ) {
                                                    if ( cl->stats.num_overlap_literals <= 51.5f ) {
                                                        if ( cl->stats.size_rel <= 0.914809703827f ) {
                                                            return 1266.0/326.8;
                                                        } else {
                                                            return 1076.0/197.7;
                                                        }
                                                    } else {
                                                        if ( cl->stats.num_overlap_literals <= 94.5f ) {
                                                            return 1252.0/466.0;
                                                        } else {
                                                            return 1736.0/470.0;
                                                        }
                                                    }
                                                } else {
                                                    if ( cl->stats.num_antecedents_rel <= 0.684085249901f ) {
                                                        if ( cl->stats.antecedents_glue_long_reds_var <= 3.52777767181f ) {
                                                            return 917.0/242.1;
                                                        } else {
                                                            return 1112.0/234.0;
                                                        }
                                                    } else {
                                                        return 1104.0/191.6;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.size_rel <= 1.56805360317f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 212.5f ) {
                                    if ( cl->stats.glue <= 9.5f ) {
                                        return 1495.0/494.2;
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.860969424248f ) {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.860798597336f ) {
                                                return 1220.0/595.1;
                                            } else {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.26706713438f ) {
                                                    return 1132.0/123.1;
                                                } else {
                                                    if ( cl->stats.antecedents_glue_long_reds_var <= 27.7827777863f ) {
                                                        if ( cl->stats.size_rel <= 0.881629705429f ) {
                                                            return 1818.0/492.2;
                                                        } else {
                                                            if ( cl->stats.num_antecedents_rel <= 0.630825281143f ) {
                                                                if ( rdb0_last_touched_diff <= 198695.0f ) {
                                                                    return 1184.0/276.4;
                                                                } else {
                                                                    return 944.0/147.3;
                                                                }
                                                            } else {
                                                                return 1143.0/143.2;
                                                            }
                                                        }
                                                    } else {
                                                        if ( cl->stats.rdb1_act_ranking_top_10 <= 6.5f ) {
                                                            return 1056.0/92.8;
                                                        } else {
                                                            return 1026.0/141.2;
                                                        }
                                                    }
                                                }
                                            }
                                        } else {
                                            if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                                if ( cl->stats.antecedents_glue_long_reds_var <= 11.6063404083f ) {
                                                    return 1505.0/300.6;
                                                } else {
                                                    return 1226.0/151.3;
                                                }
                                            } else {
                                                return 1081.0/72.6;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue <= 17.5f ) {
                                        if ( cl->stats.glue_rel_long <= 1.29237949848f ) {
                                            if ( cl->stats.dump_number <= 22.5f ) {
                                                return 1786.0/310.7;
                                            } else {
                                                return 876.0/302.6;
                                            }
                                        } else {
                                            return 1039.0/119.0;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 1.35948812962f ) {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.734369754791f ) {
                                                return 1247.0/232.0;
                                            } else {
                                                if ( cl->stats.num_antecedents_rel <= 2.07618880272f ) {
                                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 6.5f ) {
                                                        return 1270.0/78.7;
                                                    } else {
                                                        return 1363.0/173.5;
                                                    }
                                                } else {
                                                    return 1144.0/169.5;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 432.5f ) {
                                                return 1266.0/121.0;
                                            } else {
                                                return 1585.0/78.7;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.glue <= 14.5f ) {
                                    if ( rdb0_last_touched_diff <= 169909.0f ) {
                                        return 1055.0/240.1;
                                    } else {
                                        return 1068.0/137.2;
                                    }
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                        if ( cl->stats.size_rel <= 2.57131695747f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.845754861832f ) {
                                                return 1371.0/217.9;
                                            } else {
                                                if ( cl->stats.num_antecedents_rel <= 1.11111617088f ) {
                                                    return 1118.0/74.6;
                                                } else {
                                                    return 1243.0/147.3;
                                                }
                                            }
                                        } else {
                                            return 1570.0/100.9;
                                        }
                                    } else {
                                        if ( cl->stats.size_rel <= 1.97447538376f ) {
                                            return 1043.0/26.2;
                                        } else {
                                            return 1378.0/68.6;
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
        if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
            if ( cl->stats.num_total_lits_antecedents <= 45.5f ) {
                if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                    if ( rdb0_last_touched_diff <= 2497.5f ) {
                        if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                            if ( cl->stats.glue_rel_long <= 0.699353694916f ) {
                                if ( cl->stats.num_overlap_literals <= 11.5f ) {
                                    if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                                        return 355.0/3124.8;
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.034034229815f ) {
                                            return 231.0/1803.5;
                                        } else {
                                            return 232.0/2969.5;
                                        }
                                    }
                                } else {
                                    return 237.0/1823.7;
                                }
                            } else {
                                return 320.0/2207.0;
                            }
                        } else {
                            if ( cl->stats.num_antecedents_rel <= 0.0350669547915f ) {
                                return 183.0/1771.2;
                            } else {
                                if ( rdb0_last_touched_diff <= 395.5f ) {
                                    if ( cl->stats.used_for_uip_creation <= 7.5f ) {
                                        return 86.0/1940.7;
                                    } else {
                                        return 98.0/3810.7;
                                    }
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 11.5f ) {
                                        if ( rdb0_last_touched_diff <= 1748.0f ) {
                                            if ( cl->stats.size_rel <= 0.242211192846f ) {
                                                return 180.0/2174.7;
                                            } else {
                                                return 166.0/3340.7;
                                            }
                                        } else {
                                            return 200.0/2253.3;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 7585.0f ) {
                                            return 68.0/2182.7;
                                        } else {
                                            return 89.0/1791.4;
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->size() <= 10.5f ) {
                            if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0973772257566f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0250182896852f ) {
                                        return 291.0/1575.5;
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 4.5f ) {
                                            return 268.0/2039.5;
                                        } else {
                                            return 415.0/2630.6;
                                        }
                                    }
                                } else {
                                    return 377.0/1839.8;
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 16384.5f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.213151931763f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.0814015343785f ) {
                                            return 373.0/2925.1;
                                        } else {
                                            if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                                if ( rdb0_last_touched_diff <= 5691.0f ) {
                                                    return 227.0/2348.2;
                                                } else {
                                                    return 218.0/1845.8;
                                                }
                                            } else {
                                                return 237.0/3298.3;
                                            }
                                        }
                                    } else {
                                        return 243.0/2999.8;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                        return 224.0/1787.3;
                                    } else {
                                        return 295.0/1599.7;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.111911684275f ) {
                                    return 375.0/1379.8;
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.106355518103f ) {
                                        return 339.0/1888.2;
                                    } else {
                                        return 329.0/1438.4;
                                    }
                                }
                            } else {
                                if ( cl->stats.size_rel <= 0.515211045742f ) {
                                    return 325.0/2481.3;
                                } else {
                                    return 303.0/1753.1;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.543083906174f ) {
                        if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                            if ( cl->stats.num_overlap_literals_rel <= 0.0308598093688f ) {
                                return 478.0/1325.4;
                            } else {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                                    return 340.0/1787.3;
                                } else {
                                    return 628.0/2414.7;
                                }
                            }
                        } else {
                            if ( cl->stats.num_antecedents_rel <= 0.0692157000303f ) {
                                return 232.0/1565.4;
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.132672950625f ) {
                                    return 292.0/3207.5;
                                } else {
                                    return 291.0/2632.6;
                                }
                            }
                        }
                    } else {
                        return 645.0/2751.6;
                    }
                }
            } else {
                if ( cl->stats.num_total_lits_antecedents <= 625.5f ) {
                    if ( rdb0_last_touched_diff <= 2478.5f ) {
                        if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                            if ( cl->stats.num_overlap_literals_rel <= 0.0672345012426f ) {
                                return 234.0/1916.5;
                            } else {
                                if ( rdb0_last_touched_diff <= 478.0f ) {
                                    return 132.0/2138.4;
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 1.1657987833f ) {
                                        return 197.0/2059.7;
                                    } else {
                                        return 325.0/2588.2;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                return 741.0/2338.1;
                            } else {
                                if ( cl->size() <= 31.5f ) {
                                    return 251.0/2445.0;
                                } else {
                                    return 269.0/3554.5;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 31568.5f ) {
                            if ( cl->size() <= 86.5f ) {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                    if ( cl->stats.glue_rel_long <= 1.03920221329f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 12035.0f ) {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.271518498659f ) {
                                                return 358.0/1533.2;
                                            } else {
                                                return 414.0/2412.7;
                                            }
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 78.5f ) {
                                                return 440.0/1868.0;
                                            } else {
                                                return 441.0/1652.2;
                                            }
                                        }
                                    } else {
                                        return 417.0/1297.1;
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.396808326244f ) {
                                        return 467.0/2307.8;
                                    } else {
                                        return 223.0/1771.2;
                                    }
                                }
                            } else {
                                return 427.0/1226.5;
                            }
                        } else {
                            if ( cl->stats.num_antecedents_rel <= 0.379185527563f ) {
                                return 863.0/1740.9;
                            } else {
                                return 631.0/1745.0;
                            }
                        }
                    }
                } else {
                    return 490.0/1242.7;
                }
            }
        } else {
            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                    if ( cl->stats.used_for_uip_creation <= 6.5f ) {
                        if ( rdb0_last_touched_diff <= 1329.5f ) {
                            if ( cl->size() <= 12.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 2883.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.435194313526f ) {
                                        return 97.0/2287.6;
                                    } else {
                                        return 61.0/2007.2;
                                    }
                                } else {
                                    return 134.0/2499.5;
                                }
                            } else {
                                return 223.0/3826.9;
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.0276816617697f ) {
                                if ( cl->size() <= 10.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 4304.0f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 422.5f ) {
                                            return 100.0/2116.2;
                                        } else {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0331738106906f ) {
                                                return 195.0/2586.2;
                                            } else {
                                                if ( cl->stats.num_total_lits_antecedents <= 15.5f ) {
                                                    return 98.0/2172.7;
                                                } else {
                                                    return 110.0/2102.0;
                                                }
                                            }
                                        }
                                    } else {
                                        return 189.0/2237.2;
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.136380717158f ) {
                                        return 194.0/1775.2;
                                    } else {
                                        return 182.0/2017.3;
                                    }
                                }
                            } else {
                                if ( cl->stats.size_rel <= 0.685049533844f ) {
                                    if ( cl->stats.glue <= 6.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0887727066875f ) {
                                            return 168.0/1664.3;
                                        } else {
                                            return 139.0/1849.9;
                                        }
                                    } else {
                                        return 346.0/3237.8;
                                    }
                                } else {
                                    return 267.0/1970.9;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.used_for_uip_creation <= 11.5f ) {
                            if ( cl->stats.num_overlap_literals <= 14.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.0909369289875f ) {
                                    return 148.0/3141.0;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 1416.5f ) {
                                        return 75.0/2438.9;
                                    } else {
                                        return 60.0/2271.5;
                                    }
                                }
                            } else {
                                return 172.0/3231.7;
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 77.5f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.0780327171087f ) {
                                    return 93.0/2269.5;
                                } else {
                                    return 66.0/2501.5;
                                }
                            } else {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.149444445968f ) {
                                    if ( cl->size() <= 11.5f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 15.5f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 825.5f ) {
                                                return 51.0/3260.0;
                                            } else {
                                                return 71.0/2739.5;
                                            }
                                        } else {
                                            return 100.0/3651.4;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 864.0f ) {
                                            return 24.0/2943.3;
                                        } else {
                                            return 26.0/2045.6;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.555941939354f ) {
                                        return 61.0/2719.4;
                                    } else {
                                        return 85.0/2606.4;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_last_touched_diff <= 1550.5f ) {
                        if ( cl->stats.size_rel <= 0.0457346700132f ) {
                            return 45.0/2802.1;
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 16.5f ) {
                                if ( cl->stats.glue <= 7.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0906462669373f ) {
                                        return 89.0/2142.4;
                                    } else {
                                        return 63.0/2586.2;
                                    }
                                } else {
                                    return 151.0/1819.6;
                                }
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.0334384292364f ) {
                                    return 112.0/3264.0;
                                } else {
                                    if ( cl->size() <= 25.5f ) {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.234583333135f ) {
                                            if ( cl->stats.num_antecedents_rel <= 0.164120674133f ) {
                                                if ( cl->stats.used_for_uip_creation <= 44.5f ) {
                                                    return 53.0/3173.2;
                                                } else {
                                                    return 68.0/3122.8;
                                                }
                                            } else {
                                                return 57.0/4020.5;
                                            }
                                        } else {
                                            return 108.0/2858.5;
                                        }
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 99.5f ) {
                                            return 20.0/3034.1;
                                        } else {
                                            return 41.0/2630.6;
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_queue <= 0.277224898338f ) {
                            if ( cl->stats.glue <= 3.5f ) {
                                return 66.0/2017.3;
                            } else {
                                return 129.0/1829.7;
                            }
                        } else {
                            if ( cl->size() <= 7.5f ) {
                                if ( rdb0_last_touched_diff <= 621.0f ) {
                                    return 38.0/2487.4;
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.076815828681f ) {
                                        return 81.0/1942.7;
                                    } else {
                                        return 46.0/2136.3;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 12.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.506212472916f ) {
                                        return 148.0/2537.8;
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.195790827274f ) {
                                            return 67.0/2152.5;
                                        } else {
                                            return 135.0/2293.7;
                                        }
                                    }
                                } else {
                                    return 47.0/2638.7;
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.glue <= 6.5f ) {
                    if ( cl->stats.rdb1_last_touched_diff <= 1784.0f ) {
                        if ( cl->stats.used_for_uip_creation <= 5.5f ) {
                            return 141.0/2196.9;
                        } else {
                            return 90.0/2434.9;
                        }
                    } else {
                        return 268.0/2870.6;
                    }
                } else {
                    if ( cl->stats.rdb1_last_touched_diff <= 2146.0f ) {
                        return 272.0/3231.7;
                    } else {
                        return 225.0/1662.3;
                    }
                }
            }
        }
    }
}

static double estimator_should_keep_short_conf3_cluster0_7(
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
    if ( rdb0_last_touched_diff <= 25337.5f ) {
        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
            if ( rdb0_last_touched_diff <= 9349.5f ) {
                if ( cl->stats.size_rel <= 0.476171731949f ) {
                    if ( cl->stats.rdb1_last_touched_diff <= 9273.5f ) {
                        if ( cl->size() <= 8.5f ) {
                            if ( rdb0_last_touched_diff <= 1920.5f ) {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                        if ( rdb0_last_touched_diff <= 1228.5f ) {
                                            if ( cl->stats.glue <= 4.5f ) {
                                                return 209.0/3362.9;
                                            } else {
                                                return 95.0/2130.3;
                                            }
                                        } else {
                                            return 145.0/1821.6;
                                        }
                                    } else {
                                        return 116.0/2775.8;
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 661.5f ) {
                                        if ( cl->stats.size_rel <= 0.216996401548f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 2867.5f ) {
                                                if ( cl->stats.num_overlap_literals_rel <= 0.00312503403984f ) {
                                                    return 15.0/2235.2;
                                                } else {
                                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0681897699833f ) {
                                                        if ( cl->stats.size_rel <= 0.0437286496162f ) {
                                                            return 28.0/2027.4;
                                                        } else {
                                                            if ( cl->stats.antec_num_total_lits_rel <= 0.0262702386826f ) {
                                                                return 75.0/1968.9;
                                                            } else {
                                                                return 83.0/3518.2;
                                                            }
                                                        }
                                                    } else {
                                                        return 43.0/3788.5;
                                                    }
                                                }
                                            } else {
                                                return 101.0/3062.3;
                                            }
                                        } else {
                                            if ( cl->stats.dump_number <= 14.5f ) {
                                                return 55.0/3506.1;
                                            } else {
                                                return 23.0/3139.0;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.158939570189f ) {
                                            if ( cl->size() <= 4.5f ) {
                                                if ( cl->stats.used_for_uip_creation <= 14.5f ) {
                                                    return 140.0/2190.8;
                                                } else {
                                                    return 69.0/2596.3;
                                                }
                                            } else {
                                                if ( cl->stats.size_rel <= 0.23181951046f ) {
                                                    return 140.0/3774.4;
                                                } else {
                                                    return 45.0/2047.6;
                                                }
                                            }
                                        } else {
                                            if ( cl->size() <= 5.5f ) {
                                                return 42.0/2606.4;
                                            } else {
                                                return 80.0/2422.8;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 6275.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.0820485204458f ) {
                                        if ( rdb0_last_touched_diff <= 3459.0f ) {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.0259203575552f ) {
                                                return 87.0/1981.0;
                                            } else {
                                                return 178.0/2116.2;
                                            }
                                        } else {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0171145759523f ) {
                                                return 162.0/1860.0;
                                            } else {
                                                return 165.0/2104.1;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 4895.5f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0347255915403f ) {
                                                return 162.0/2354.2;
                                            } else {
                                                if ( rdb0_last_touched_diff <= 4338.5f ) {
                                                    if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                                        return 156.0/2207.0;
                                                    } else {
                                                        if ( cl->stats.glue <= 4.5f ) {
                                                            return 73.0/3487.9;
                                                        } else {
                                                            return 88.0/2473.2;
                                                        }
                                                    }
                                                } else {
                                                    return 198.0/3427.4;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 0.412878483534f ) {
                                                return 153.0/1872.1;
                                            } else {
                                                return 182.0/1855.9;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0368612334132f ) {
                                        return 334.0/2832.3;
                                    } else {
                                        if ( cl->stats.size_rel <= 0.266579926014f ) {
                                            return 206.0/2344.1;
                                        } else {
                                            return 136.0/2176.7;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 778.0f ) {
                                if ( rdb0_last_touched_diff <= 2516.0f ) {
                                    if ( cl->stats.used_for_uip_creation <= 11.5f ) {
                                        return 186.0/2781.9;
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.653333306313f ) {
                                            if ( cl->stats.glue_rel_queue <= 0.288739442825f ) {
                                                return 62.0/2283.6;
                                            } else {
                                                if ( cl->stats.used_for_uip_creation <= 35.5f ) {
                                                    return 56.0/2586.2;
                                                } else {
                                                    return 23.0/3780.5;
                                                }
                                            }
                                        } else {
                                            return 70.0/1975.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.515331149101f ) {
                                        return 169.0/2221.1;
                                    } else {
                                        return 174.0/1811.6;
                                    }
                                }
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 5.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.817969441414f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 68.5f ) {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 5.5f ) {
                                                if ( cl->stats.num_antecedents_rel <= 0.0853838324547f ) {
                                                    return 268.0/1630.0;
                                                } else {
                                                    return 417.0/3340.7;
                                                }
                                            } else {
                                                return 204.0/2699.2;
                                            }
                                        } else {
                                            return 298.0/1652.2;
                                        }
                                    } else {
                                        return 309.0/1835.8;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0197470467538f ) {
                                        if ( cl->stats.used_for_uip_creation <= 15.5f ) {
                                            return 123.0/2168.6;
                                        } else {
                                            return 61.0/2781.9;
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 2466.5f ) {
                                            if ( cl->stats.used_for_uip_creation <= 10.5f ) {
                                                return 157.0/2422.8;
                                            } else {
                                                if ( cl->stats.glue_rel_queue <= 0.41644987464f ) {
                                                    return 78.0/2356.2;
                                                } else {
                                                    if ( cl->stats.used_for_uip_creation <= 23.5f ) {
                                                        return 58.0/1928.6;
                                                    } else {
                                                        return 23.0/2021.4;
                                                    }
                                                }
                                            }
                                        } else {
                                            return 288.0/3009.8;
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 23.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0325054675341f ) {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                        return 302.0/1517.0;
                                    } else {
                                        return 474.0/1593.7;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 32902.0f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.417772054672f ) {
                                            return 210.0/1684.5;
                                        } else {
                                            return 335.0/2146.4;
                                        }
                                    } else {
                                        return 361.0/1745.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.dump_number <= 11.5f ) {
                                    return 619.0/2747.6;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 36623.0f ) {
                                        return 411.0/1767.2;
                                    } else {
                                        return 461.0/1272.9;
                                    }
                                }
                            }
                        } else {
                            if ( cl->size() <= 8.5f ) {
                                if ( cl->stats.used_for_uip_creation <= 6.5f ) {
                                    if ( rdb0_last_touched_diff <= 1861.0f ) {
                                        return 226.0/3090.5;
                                    } else {
                                        if ( cl->stats.size_rel <= 0.194571793079f ) {
                                            return 311.0/1892.2;
                                        } else {
                                            return 199.0/1789.4;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 25750.5f ) {
                                        if ( cl->stats.dump_number <= 18.5f ) {
                                            return 192.0/2995.7;
                                        } else {
                                            return 92.0/1896.3;
                                        }
                                    } else {
                                        return 240.0/2469.2;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.415421903133f ) {
                                    return 332.0/3260.0;
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0939773544669f ) {
                                        return 287.0/1878.1;
                                    } else {
                                        if ( rdb0_last_touched_diff <= 2207.5f ) {
                                            return 168.0/2689.1;
                                        } else {
                                            return 305.0/1577.5;
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 34084.0f ) {
                                if ( cl->stats.size_rel <= 0.887023746967f ) {
                                    if ( cl->stats.num_overlap_literals <= 17.5f ) {
                                        return 370.0/2432.9;
                                    } else {
                                        return 391.0/1841.8;
                                    }
                                } else {
                                    return 508.0/1876.1;
                                }
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 60.5f ) {
                                        return 469.0/1387.9;
                                    } else {
                                        return 527.0/1043.0;
                                    }
                                } else {
                                    return 533.0/2013.3;
                                }
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 157.5f ) {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 4.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.889511704445f ) {
                                        if ( cl->size() <= 21.5f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.146337240934f ) {
                                                return 196.0/1706.7;
                                            } else {
                                                return 192.0/1650.2;
                                            }
                                        } else {
                                            return 291.0/1736.9;
                                        }
                                    } else {
                                        return 326.0/1595.7;
                                    }
                                } else {
                                    return 284.0/3494.0;
                                }
                            } else {
                                return 369.0/1694.6;
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 5680.5f ) {
                            if ( cl->stats.used_for_uip_creation <= 12.5f ) {
                                if ( cl->stats.dump_number <= 3.5f ) {
                                    return 268.0/3013.9;
                                } else {
                                    if ( cl->stats.glue <= 9.5f ) {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 5.5f ) {
                                            return 157.0/2035.5;
                                        } else {
                                            return 90.0/2146.4;
                                        }
                                    } else {
                                        return 172.0/3734.1;
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 2804.0f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 1800.5f ) {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 33.5f ) {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.13066893816f ) {
                                                return 33.0/2265.5;
                                            } else {
                                                return 58.0/2265.5;
                                            }
                                        } else {
                                            return 29.0/3312.4;
                                        }
                                    } else {
                                        return 83.0/2798.0;
                                    }
                                } else {
                                    return 153.0/2015.3;
                                }
                            }
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 7.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 79.5f ) {
                                    if ( cl->size() <= 15.5f ) {
                                        return 194.0/1803.5;
                                    } else {
                                        return 224.0/1831.7;
                                    }
                                } else {
                                    return 323.0/1700.6;
                                }
                            } else {
                                if ( cl->stats.dump_number <= 16.5f ) {
                                    if ( cl->stats.dump_number <= 5.5f ) {
                                        return 129.0/1920.5;
                                    } else {
                                        return 94.0/2005.2;
                                    }
                                } else {
                                    return 195.0/2352.2;
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.glue <= 8.5f ) {
                    if ( cl->stats.num_total_lits_antecedents <= 15.5f ) {
                        if ( rdb0_last_touched_diff <= 13190.5f ) {
                            if ( cl->stats.dump_number <= 20.5f ) {
                                return 313.0/2547.9;
                            } else {
                                return 371.0/2081.9;
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 10.5f ) {
                                return 341.0/1944.7;
                            } else {
                                if ( cl->stats.size_rel <= 0.240462481976f ) {
                                    return 412.0/1738.9;
                                } else {
                                    return 285.0/1781.3;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.dump_number <= 2.5f ) {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                return 887.0/2152.5;
                            } else {
                                return 590.0/2723.4;
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 15835.0f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 23.5f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                                        return 428.0/1993.1;
                                    } else {
                                        return 230.0/1670.3;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 5.5f ) {
                                        if ( cl->size() <= 9.5f ) {
                                            return 344.0/1607.8;
                                        } else {
                                            if ( rdb0_last_touched_diff <= 12169.0f ) {
                                                return 371.0/1381.9;
                                            } else {
                                                return 435.0/1256.8;
                                            }
                                        }
                                    } else {
                                        return 274.0/1759.1;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue <= 5.5f ) {
                                    return 728.0/2681.0;
                                } else {
                                    if ( cl->stats.dump_number <= 11.5f ) {
                                        return 370.0/1398.0;
                                    } else {
                                        return 781.0/2092.0;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                        if ( cl->stats.glue_rel_queue <= 0.916438579559f ) {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.419987857342f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.494897961617f ) {
                                    return 487.0/1472.6;
                                } else {
                                    return 757.0/1718.8;
                                }
                            } else {
                                return 710.0/1182.2;
                            }
                        } else {
                            if ( cl->stats.num_antecedents_rel <= 0.327868491411f ) {
                                return 819.0/1232.6;
                            } else {
                                if ( cl->stats.glue_rel_long <= 1.0624576807f ) {
                                    return 769.0/714.1;
                                } else {
                                    return 1105.0/579.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.num_overlap_literals <= 73.5f ) {
                            if ( rdb0_last_touched_diff <= 14109.0f ) {
                                return 595.0/2475.3;
                            } else {
                                return 430.0/1383.9;
                            }
                        } else {
                            return 478.0/1270.9;
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.num_overlap_literals <= 91.5f ) {
                if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                    if ( cl->stats.antecedents_glue_long_reds_var <= 2.85825896263f ) {
                        if ( cl->stats.glue_rel_long <= 0.72376614809f ) {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 47.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0608904138207f ) {
                                            if ( cl->size() <= 4.5f ) {
                                                return 408.0/1337.5;
                                            } else {
                                                return 731.0/1654.2;
                                            }
                                        } else {
                                            if ( cl->stats.num_overlap_literals <= 7.5f ) {
                                                if ( cl->stats.num_overlap_literals_rel <= 0.0770249739289f ) {
                                                    if ( cl->stats.num_overlap_literals <= 3.5f ) {
                                                        return 411.0/1860.0;
                                                    } else {
                                                        return 686.0/2445.0;
                                                    }
                                                } else {
                                                    return 277.0/1504.9;
                                                }
                                            } else {
                                                if ( cl->size() <= 9.5f ) {
                                                    return 680.0/2761.7;
                                                } else {
                                                    return 656.0/1878.1;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.487532973289f ) {
                                            return 469.0/1127.7;
                                        } else {
                                            return 736.0/1379.8;
                                        }
                                    }
                                } else {
                                    return 727.0/1718.8;
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 12931.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.462197095156f ) {
                                        return 375.0/3397.2;
                                    } else {
                                        return 347.0/2140.4;
                                    }
                                } else {
                                    if ( cl->size() <= 7.5f ) {
                                        return 445.0/2483.3;
                                    } else {
                                        return 571.0/2574.1;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.dump_number <= 2.5f ) {
                                if ( cl->size() <= 11.5f ) {
                                    return 602.0/1652.2;
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.921846389771f ) {
                                        return 628.0/811.0;
                                    } else {
                                        return 1175.0/1034.9;
                                    }
                                }
                            } else {
                                if ( cl->size() <= 11.5f ) {
                                    return 591.0/2237.2;
                                } else {
                                    if ( cl->stats.size_rel <= 0.681357860565f ) {
                                        return 595.0/1767.2;
                                    } else {
                                        return 817.0/1866.0;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_long <= 1.00824594498f ) {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                if ( cl->stats.dump_number <= 2.5f ) {
                                    return 1028.0/1059.1;
                                } else {
                                    return 748.0/1369.8;
                                }
                            } else {
                                return 773.0/1894.3;
                            }
                        } else {
                            if ( cl->stats.dump_number <= 1.5f ) {
                                return 1000.0/429.7;
                            } else {
                                return 893.0/857.4;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                        if ( cl->stats.glue_rel_long <= 0.559355258942f ) {
                            if ( cl->stats.used_for_uip_creation <= 5.5f ) {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 4.5f ) {
                                    return 219.0/1664.3;
                                } else {
                                    return 155.0/2065.7;
                                }
                            } else {
                                return 171.0/3711.9;
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.13066893816f ) {
                                return 230.0/2854.5;
                            } else {
                                return 261.0/1761.1;
                            }
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 29.5f ) {
                            return 307.0/2029.4;
                        } else {
                            return 428.0/2051.6;
                        }
                    }
                }
            } else {
                if ( rdb0_last_touched_diff <= 10028.0f ) {
                    return 602.0/1964.9;
                } else {
                    if ( cl->stats.glue_rel_queue <= 0.974585771561f ) {
                        if ( cl->size() <= 57.5f ) {
                            if ( cl->stats.glue_rel_queue <= 0.783348441124f ) {
                                return 524.0/1032.9;
                            } else {
                                return 741.0/734.3;
                            }
                        } else {
                            return 810.0/498.3;
                        }
                    } else {
                        if ( cl->stats.glue <= 13.5f ) {
                            return 1044.0/603.2;
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 353.5f ) {
                                return 1326.0/389.3;
                            } else {
                                if ( cl->stats.dump_number <= 1.5f ) {
                                    return 1507.0/129.1;
                                } else {
                                    return 873.0/242.1;
                                }
                            }
                        }
                    }
                }
            }
        }
    } else {
        if ( cl->stats.antecedents_glue_long_reds_var <= 4.09967851639f ) {
            if ( rdb0_last_touched_diff <= 95105.5f ) {
                if ( cl->stats.glue_rel_long <= 0.846135020256f ) {
                    if ( cl->stats.num_total_lits_antecedents <= 38.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 39054.5f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 18.5f ) {
                                if ( rdb0_last_touched_diff <= 30419.0f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0696810409427f ) {
                                        return 504.0/1458.5;
                                    } else {
                                        return 481.0/1954.8;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.054611261934f ) {
                                        if ( cl->stats.size_rel <= 0.104168459773f ) {
                                            return 410.0/1460.5;
                                        } else {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0261719636619f ) {
                                                return 596.0/1151.9;
                                            } else {
                                                return 942.0/2249.3;
                                            }
                                        }
                                    } else {
                                        if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                                            return 391.0/1454.5;
                                        } else {
                                            return 422.0/1238.6;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->size() <= 10.5f ) {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                        return 739.0/2128.3;
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.459840774536f ) {
                                            return 619.0/1557.4;
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 26619.5f ) {
                                                return 505.0/1131.7;
                                            } else {
                                                return 581.0/1038.9;
                                            }
                                        }
                                    }
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                        return 644.0/1527.1;
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 22782.5f ) {
                                            return 625.0/1385.9;
                                        } else {
                                            if ( cl->stats.glue <= 7.5f ) {
                                                return 754.0/996.6;
                                            } else {
                                                return 549.0/938.1;
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.0773591026664f ) {
                                    return 476.0/1210.4;
                                } else {
                                    return 735.0/1291.1;
                                }
                            } else {
                                if ( cl->size() <= 9.5f ) {
                                    if ( cl->stats.size_rel <= 0.137546271086f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0235136151314f ) {
                                            return 600.0/944.1;
                                        } else {
                                            return 497.0/1103.5;
                                        }
                                    } else {
                                        if ( cl->stats.dump_number <= 49.5f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0624502673745f ) {
                                                if ( cl->stats.num_antecedents_rel <= 0.0981423035264f ) {
                                                    return 600.0/978.4;
                                                } else {
                                                    return 778.0/865.4;
                                                }
                                            } else {
                                                if ( cl->stats.size_rel <= 0.286295115948f ) {
                                                    return 778.0/1022.8;
                                                } else {
                                                    return 986.0/1767.2;
                                                }
                                            }
                                        } else {
                                            return 568.0/1103.5;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue <= 7.5f ) {
                                        if ( cl->stats.size_rel <= 0.471298396587f ) {
                                            return 839.0/1077.2;
                                        } else {
                                            return 1058.0/1160.0;
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 69690.0f ) {
                                            return 632.0/788.8;
                                        } else {
                                            return 784.0/607.2;
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                            if ( cl->stats.size_rel <= 0.778267025948f ) {
                                if ( rdb0_last_touched_diff <= 37020.0f ) {
                                    return 810.0/1821.6;
                                } else {
                                    return 1077.0/1664.3;
                                }
                            } else {
                                return 720.0/853.3;
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.13066893816f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 40783.0f ) {
                                    if ( cl->stats.dump_number <= 4.5f ) {
                                        return 731.0/907.8;
                                    } else {
                                        return 798.0/1890.2;
                                    }
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                        return 824.0/691.9;
                                    } else {
                                        return 845.0/1200.3;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue <= 8.5f ) {
                                    if ( cl->size() <= 11.5f ) {
                                        return 1022.0/1777.3;
                                    } else {
                                        if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                                            if ( cl->stats.glue <= 6.5f ) {
                                                return 818.0/958.2;
                                            } else {
                                                return 837.0/819.0;
                                            }
                                        } else {
                                            return 1302.0/940.1;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 67.5f ) {
                                        return 765.0/710.1;
                                    } else {
                                        if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                                            return 870.0/579.0;
                                        } else {
                                            return 889.0/365.1;
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_overlap_literals <= 38.5f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 25.5f ) {
                            if ( rdb0_last_touched_diff <= 49253.0f ) {
                                return 556.0/1410.1;
                            } else {
                                return 604.0/932.0;
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 48750.0f ) {
                                if ( cl->size() <= 14.5f ) {
                                    return 672.0/1137.8;
                                } else {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                                        return 869.0/1107.5;
                                    } else {
                                        return 1097.0/911.8;
                                    }
                                }
                            } else {
                                if ( cl->stats.dump_number <= 9.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.971929073334f ) {
                                        return 848.0/574.9;
                                    } else {
                                        return 1594.0/706.1;
                                    }
                                } else {
                                    return 912.0/1077.2;
                                }
                            }
                        }
                    } else {
                        if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                            if ( cl->stats.dump_number <= 9.5f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.757872164249f ) {
                                    return 1084.0/635.5;
                                } else {
                                    return 1295.0/451.9;
                                }
                            } else {
                                return 767.0/964.3;
                            }
                        } else {
                            if ( cl->stats.dump_number <= 6.5f ) {
                                return 1332.0/320.8;
                            } else {
                                return 953.0/421.6;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.rdb1_last_touched_diff <= 148741.0f ) {
                    if ( cl->stats.size_rel <= 0.612889766693f ) {
                        if ( cl->stats.size_rel <= 0.177353709936f ) {
                            if ( cl->stats.glue_rel_long <= 0.404227793217f ) {
                                return 1208.0/1628.0;
                            } else {
                                return 1109.0/1166.0;
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 133558.0f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 18.5f ) {
                                    return 1043.0/1351.6;
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.781043887138f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 29.5f ) {
                                            return 1078.0/861.4;
                                        } else {
                                            if ( cl->stats.num_overlap_literals <= 25.5f ) {
                                                return 867.0/899.7;
                                            } else {
                                                return 761.0/633.4;
                                            }
                                        }
                                    } else {
                                        return 1176.0/661.7;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.62451249361f ) {
                                    return 1177.0/954.2;
                                } else {
                                    return 1074.0/566.9;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.num_antecedents_rel <= 0.281029939651f ) {
                            if ( cl->size() <= 24.5f ) {
                                return 1139.0/813.0;
                            } else {
                                return 1383.0/645.5;
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.00802091788501f ) {
                                return 1100.0/564.9;
                            } else {
                                if ( cl->stats.size_rel <= 0.870415568352f ) {
                                    return 1171.0/581.0;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 111953.0f ) {
                                        return 1134.0/308.7;
                                    } else {
                                        return 1284.0/228.0;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.glue <= 7.5f ) {
                        if ( cl->size() <= 7.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 272420.5f ) {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 6.5f ) {
                                    return 857.0/1085.3;
                                } else {
                                    return 1172.0/1105.5;
                                }
                            } else {
                                if ( cl->stats.size_rel <= 0.116721570492f ) {
                                    return 957.0/770.6;
                                } else {
                                    return 1114.0/631.4;
                                }
                            }
                        } else {
                            if ( cl->size() <= 12.5f ) {
                                if ( cl->stats.num_overlap_literals <= 6.5f ) {
                                    return 1215.0/815.0;
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 7.5f ) {
                                        if ( rdb0_last_touched_diff <= 270854.5f ) {
                                            return 907.0/472.1;
                                        } else {
                                            return 942.0/324.8;
                                        }
                                    } else {
                                        return 1448.0/776.7;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.439061045647f ) {
                                    return 1033.0/524.5;
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.670614898205f ) {
                                        return 1367.0/369.2;
                                    } else {
                                        return 1299.0/423.6;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->size() <= 13.5f ) {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.292420566082f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.14769884944f ) {
                                    return 803.0/514.4;
                                } else {
                                    return 931.0/512.4;
                                }
                            } else {
                                return 976.0/306.6;
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals <= 17.5f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.333591580391f ) {
                                    if ( rdb0_last_touched_diff <= 294699.5f ) {
                                        if ( cl->stats.glue <= 12.5f ) {
                                            return 1115.0/538.6;
                                        } else {
                                            return 873.0/324.8;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 503851.0f ) {
                                            return 1142.0/308.7;
                                        } else {
                                            return 898.0/347.0;
                                        }
                                    }
                                } else {
                                    return 1071.0/250.1;
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 1.01130700111f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.0874804705381f ) {
                                        return 1537.0/558.8;
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 299581.5f ) {
                                            if ( rdb0_act_ranking_top_10 <= 7.5f ) {
                                                return 1523.0/514.4;
                                            } else {
                                                return 1098.0/238.0;
                                            }
                                        } else {
                                            return 1790.0/330.8;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.0863223150373f ) {
                                        return 922.0/203.7;
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.518312335014f ) {
                                            return 1149.0/183.6;
                                        } else {
                                            return 1747.0/203.7;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.glue_rel_long <= 0.876702308655f ) {
                if ( cl->size() <= 19.5f ) {
                    if ( rdb0_last_touched_diff <= 118617.0f ) {
                        if ( cl->stats.glue <= 8.5f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                                return 916.0/1529.1;
                            } else {
                                return 929.0/1057.1;
                            }
                        } else {
                            if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                                return 879.0/1101.5;
                            } else {
                                return 1190.0/817.0;
                            }
                        }
                    } else {
                        if ( cl->stats.glue <= 8.5f ) {
                            return 1211.0/673.8;
                        } else {
                            return 1573.0/566.9;
                        }
                    }
                } else {
                    if ( cl->size() <= 105.5f ) {
                        if ( cl->stats.glue_rel_queue <= 0.8738322258f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                                if ( rdb0_last_touched_diff <= 58009.5f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.243027448654f ) {
                                        return 689.0/825.1;
                                    } else {
                                        return 863.0/909.8;
                                    }
                                } else {
                                    return 1529.0/1109.5;
                                }
                            } else {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 8.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.60060530901f ) {
                                        return 967.0/530.6;
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 84.5f ) {
                                            return 1081.0/480.1;
                                        } else {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.56926715374f ) {
                                                return 890.0/353.0;
                                            } else {
                                                return 955.0/284.4;
                                            }
                                        }
                                    }
                                } else {
                                    return 1070.0/217.9;
                                }
                            }
                        } else {
                            if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                return 1086.0/528.5;
                            } else {
                                return 1086.0/280.4;
                            }
                        }
                    } else {
                        return 1616.0/405.5;
                    }
                }
            } else {
                if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                    if ( cl->size() <= 69.5f ) {
                        if ( rdb0_last_touched_diff <= 69213.5f ) {
                            if ( cl->stats.size_rel <= 0.643858015537f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.883749246597f ) {
                                    return 1038.0/974.4;
                                } else {
                                    return 866.0/387.3;
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 1.09921216965f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.493209898472f ) {
                                        return 754.0/645.5;
                                    } else {
                                        return 1051.0/536.6;
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 162.5f ) {
                                        return 1161.0/492.2;
                                    } else {
                                        return 1054.0/260.2;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.989500284195f ) {
                                return 872.0/316.7;
                            } else {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 16.4782981873f ) {
                                    return 1122.0/340.9;
                                } else {
                                    return 1210.0/171.5;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_queue <= 1.033608675f ) {
                            return 966.0/320.8;
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 715.0f ) {
                                return 1519.0/272.3;
                            } else {
                                return 1000.0/56.5;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.glue <= 13.5f ) {
                        if ( rdb0_last_touched_diff <= 123263.5f ) {
                            if ( cl->stats.dump_number <= 10.5f ) {
                                if ( cl->stats.glue <= 10.5f ) {
                                    return 1654.0/637.5;
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                        return 1447.0/345.0;
                                    } else {
                                        return 1182.0/225.9;
                                    }
                                }
                            } else {
                                return 891.0/498.3;
                            }
                        } else {
                            if ( cl->stats.glue <= 9.5f ) {
                                return 1103.0/320.8;
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 170406.0f ) {
                                    return 1651.0/363.1;
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.832571327686f ) {
                                        if ( cl->stats.dump_number <= 29.5f ) {
                                            return 937.0/139.2;
                                        } else {
                                            return 920.0/232.0;
                                        }
                                    } else {
                                        return 1322.0/137.2;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue <= 18.5f ) {
                            if ( rdb0_last_touched_diff <= 46109.0f ) {
                                return 969.0/230.0;
                            } else {
                                if ( cl->stats.glue_rel_long <= 1.26202070713f ) {
                                    if ( cl->stats.num_overlap_literals <= 196.5f ) {
                                        if ( cl->stats.dump_number <= 27.5f ) {
                                            if ( cl->size() <= 27.5f ) {
                                                return 1650.0/159.4;
                                            } else {
                                                return 1248.0/217.9;
                                            }
                                        } else {
                                            return 1055.0/252.2;
                                        }
                                    } else {
                                        return 1236.0/286.5;
                                    }
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 1.62480950356f ) {
                                        if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                            return 1253.0/92.8;
                                        } else {
                                            return 1565.0/169.5;
                                        }
                                    } else {
                                        return 962.0/141.2;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 40087.5f ) {
                                if ( cl->stats.glue_rel_queue <= 1.15008795261f ) {
                                    return 1260.0/294.5;
                                } else {
                                    return 1802.0/232.0;
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.935598373413f ) {
                                    return 933.0/169.5;
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.844287157059f ) {
                                        if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                            if ( cl->stats.glue_rel_long <= 1.19112467766f ) {
                                                return 1043.0/113.0;
                                            } else {
                                                return 1172.0/66.6;
                                            }
                                        } else {
                                            if ( rdb0_act_ranking_top_10 <= 7.5f ) {
                                                return 1741.0/256.2;
                                            } else {
                                                if ( cl->stats.glue <= 38.5f ) {
                                                    return 1557.0/96.8;
                                                } else {
                                                    return 1168.0/175.5;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.glue <= 21.5f ) {
                                            return 1730.0/197.7;
                                        } else {
                                            if ( rdb0_last_touched_diff <= 101539.0f ) {
                                                if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                                    return 1275.0/74.6;
                                                } else {
                                                    return 1086.0/137.2;
                                                }
                                            } else {
                                                if ( cl->stats.size_rel <= 1.15649104118f ) {
                                                    return 1832.0/151.3;
                                                } else {
                                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 5.5f ) {
                                                        return 1121.0/16.1;
                                                    } else {
                                                        if ( cl->stats.num_total_lits_antecedents <= 478.0f ) {
                                                            return 993.0/62.5;
                                                        } else {
                                                            return 1548.0/42.4;
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
        }
    }
}

static double estimator_should_keep_short_conf3_cluster0_8(
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
        if ( cl->stats.rdb1_last_touched_diff <= 13726.5f ) {
            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                if ( rdb0_last_touched_diff <= 9797.5f ) {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 4.5f ) {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.00732975499704f ) {
                            if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                if ( cl->stats.num_overlap_literals <= 7.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 18.5f ) {
                                        if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                                            if ( cl->stats.size_rel <= 0.260766267776f ) {
                                                return 358.0/2549.9;
                                            } else {
                                                return 169.0/1805.5;
                                            }
                                        } else {
                                            return 323.0/3145.0;
                                        }
                                    } else {
                                        return 274.0/1638.1;
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.162857472897f ) {
                                        return 319.0/1585.6;
                                    } else {
                                        return 385.0/2709.3;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.0690499991179f ) {
                                    if ( rdb0_last_touched_diff <= 2274.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.024297600612f ) {
                                            return 132.0/1807.5;
                                        } else {
                                            return 136.0/2390.5;
                                        }
                                    } else {
                                        return 308.0/2366.3;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 4.5f ) {
                                        if ( rdb0_last_touched_diff <= 1874.0f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0349713787436f ) {
                                                return 77.0/2198.9;
                                            } else {
                                                return 55.0/2350.2;
                                            }
                                        } else {
                                            return 256.0/3758.3;
                                        }
                                    } else {
                                        if ( cl->stats.used_for_uip_creation <= 8.5f ) {
                                            if ( cl->size() <= 14.5f ) {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.116646528244f ) {
                                                    return 219.0/2317.9;
                                                } else {
                                                    if ( cl->stats.glue_rel_long <= 0.492376148701f ) {
                                                        return 125.0/2015.3;
                                                    } else {
                                                        return 146.0/1864.0;
                                                    }
                                                }
                                            } else {
                                                return 228.0/1975.0;
                                            }
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.260191321373f ) {
                                                if ( cl->stats.used_for_uip_creation <= 16.5f ) {
                                                    return 112.0/2283.6;
                                                } else {
                                                    return 71.0/2077.8;
                                                }
                                            } else {
                                                return 65.0/2305.8;
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->size() <= 10.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 20.5f ) {
                                    if ( rdb0_last_touched_diff <= 3030.5f ) {
                                        return 146.0/2295.7;
                                    } else {
                                        return 178.0/1773.2;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.145850390196f ) {
                                        return 337.0/2783.9;
                                    } else {
                                        if ( rdb0_last_touched_diff <= 2429.0f ) {
                                            return 141.0/2017.3;
                                        } else {
                                            return 239.0/1914.4;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0629403293133f ) {
                                        return 436.0/1890.2;
                                    } else {
                                        if ( cl->stats.size_rel <= 1.08903551102f ) {
                                            if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                                                return 514.0/2614.4;
                                            } else {
                                                if ( rdb0_last_touched_diff <= 3617.0f ) {
                                                    return 255.0/2398.6;
                                                } else {
                                                    return 327.0/1866.0;
                                                }
                                            }
                                        } else {
                                            return 338.0/1490.8;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 0.730455040932f ) {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                            return 203.0/2265.5;
                                        } else {
                                            return 190.0/2796.0;
                                        }
                                    } else {
                                        return 262.0/2620.5;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 12.5f ) {
                            if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                if ( cl->stats.size_rel <= 0.745569705963f ) {
                                    if ( rdb0_last_touched_diff <= 2379.0f ) {
                                        if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                            return 189.0/2767.8;
                                        } else {
                                            return 154.0/2892.8;
                                        }
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 25.5f ) {
                                            if ( cl->stats.size_rel <= 0.260688841343f ) {
                                                return 241.0/2499.5;
                                            } else {
                                                return 114.0/1999.2;
                                            }
                                        } else {
                                            return 303.0/2838.4;
                                        }
                                    }
                                } else {
                                    return 236.0/1646.1;
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 4705.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 27.5f ) {
                                        if ( cl->stats.used_for_uip_creation <= 10.5f ) {
                                            if ( cl->stats.num_antecedents_rel <= 0.0866377204657f ) {
                                                return 111.0/2120.2;
                                            } else {
                                                if ( cl->stats.num_antecedents_rel <= 0.153720647097f ) {
                                                    return 62.0/2015.3;
                                                } else {
                                                    return 50.0/2116.2;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.0594248697162f ) {
                                                return 114.0/3726.0;
                                            } else {
                                                if ( cl->stats.num_total_lits_antecedents <= 13.5f ) {
                                                    return 18.0/2130.3;
                                                } else {
                                                    return 56.0/3082.5;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 1742.5f ) {
                                            if ( cl->stats.used_for_uip_creation <= 16.5f ) {
                                                if ( rdb0_last_touched_diff <= 1129.0f ) {
                                                    return 98.0/1942.7;
                                                } else {
                                                    return 102.0/1914.4;
                                                }
                                            } else {
                                                return 48.0/2467.2;
                                            }
                                        } else {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0618218630552f ) {
                                                return 119.0/1916.5;
                                            } else {
                                                return 157.0/3500.1;
                                            }
                                        }
                                    }
                                } else {
                                    return 329.0/3126.8;
                                }
                            }
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 7.5f ) {
                                if ( rdb0_last_touched_diff <= 4756.0f ) {
                                    if ( cl->stats.glue_rel_long <= 0.321298241615f ) {
                                        return 128.0/2388.5;
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.107658654451f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0402842238545f ) {
                                                return 61.0/1946.7;
                                            } else {
                                                return 61.0/2340.1;
                                            }
                                        } else {
                                            return 112.0/2013.3;
                                        }
                                    }
                                } else {
                                    return 268.0/3318.5;
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.143176183105f ) {
                                    return 149.0/3366.9;
                                } else {
                                    if ( cl->stats.size_rel <= 0.0695642530918f ) {
                                        if ( cl->stats.dump_number <= 12.5f ) {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 37.5f ) {
                                                return 26.0/2152.5;
                                            } else {
                                                return 14.0/2219.1;
                                            }
                                        } else {
                                            return 69.0/2812.1;
                                        }
                                    } else {
                                        if ( cl->stats.used_for_uip_creation <= 14.5f ) {
                                            if ( cl->stats.num_overlap_literals <= 6.5f ) {
                                                return 85.0/3776.4;
                                            } else {
                                                if ( rdb0_last_touched_diff <= 1905.5f ) {
                                                    return 76.0/2878.7;
                                                } else {
                                                    return 121.0/1876.1;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 0.296010315418f ) {
                                                if ( cl->stats.rdb1_used_for_uip_creation <= 76.5f ) {
                                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0594927594066f ) {
                                                        return 83.0/2295.7;
                                                    } else {
                                                        return 51.0/3175.3;
                                                    }
                                                } else {
                                                    return 108.0/1985.0;
                                                }
                                            } else {
                                                if ( cl->stats.size_rel <= 0.263739407063f ) {
                                                    if ( cl->stats.num_total_lits_antecedents <= 51.5f ) {
                                                        if ( cl->stats.glue_rel_long <= 0.500934839249f ) {
                                                            if ( cl->stats.rdb1_used_for_uip_creation <= 32.5f ) {
                                                                return 76.0/2725.4;
                                                            } else {
                                                                return 70.0/3818.8;
                                                            }
                                                        } else {
                                                            return 104.0/3161.1;
                                                        }
                                                    } else {
                                                        return 32.0/2249.3;
                                                    }
                                                } else {
                                                    if ( rdb0_last_touched_diff <= 3111.0f ) {
                                                        if ( cl->stats.num_overlap_literals <= 14.5f ) {
                                                            if ( cl->stats.rdb1_used_for_uip_creation <= 25.5f ) {
                                                                return 35.0/2644.7;
                                                            } else {
                                                                if ( cl->stats.size_rel <= 0.420779287815f ) {
                                                                    return 22.0/2455.1;
                                                                } else {
                                                                    return 10.0/2340.1;
                                                                }
                                                            }
                                                        } else {
                                                            if ( cl->stats.num_antecedents_rel <= 0.204802542925f ) {
                                                                return 65.0/2273.5;
                                                            } else {
                                                                if ( cl->stats.rdb1_used_for_uip_creation <= 35.5f ) {
                                                                    return 27.0/2023.4;
                                                                } else {
                                                                    return 11.0/2652.8;
                                                                }
                                                            }
                                                        }
                                                    } else {
                                                        return 100.0/1872.1;
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
                    if ( cl->stats.antecedents_glue_long_reds_var <= 6.97899341583f ) {
                        if ( cl->stats.glue <= 8.5f ) {
                            if ( cl->stats.num_overlap_literals <= 9.5f ) {
                                if ( cl->size() <= 8.5f ) {
                                    if ( rdb0_last_touched_diff <= 17325.0f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0840541124344f ) {
                                            if ( cl->stats.glue_rel_queue <= 0.369360566139f ) {
                                                return 522.0/2479.3;
                                            } else {
                                                return 345.0/2128.3;
                                            }
                                        } else {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0772802978754f ) {
                                                return 242.0/1906.4;
                                            } else {
                                                return 301.0/1700.6;
                                            }
                                        }
                                    } else {
                                        return 607.0/2739.5;
                                    }
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.236111104488f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.122730389237f ) {
                                            return 339.0/1500.9;
                                        } else {
                                            return 345.0/1424.2;
                                        }
                                    } else {
                                        return 471.0/1414.1;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.702948689461f ) {
                                    if ( cl->size() <= 8.5f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 28.5f ) {
                                            return 357.0/1674.4;
                                        } else {
                                            return 348.0/1977.0;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 4409.0f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 1886.5f ) {
                                                return 304.0/1375.8;
                                            } else {
                                                return 391.0/1416.2;
                                            }
                                        } else {
                                            if ( cl->stats.num_overlap_literals <= 21.5f ) {
                                                return 469.0/1291.1;
                                            } else {
                                                return 422.0/1414.1;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 6.5f ) {
                                        return 431.0/1325.4;
                                    } else {
                                        return 411.0/1456.5;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                if ( cl->stats.dump_number <= 2.5f ) {
                                    return 803.0/903.8;
                                } else {
                                    return 812.0/2029.4;
                                }
                            } else {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 6.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 3757.0f ) {
                                        return 419.0/1383.9;
                                    } else {
                                        return 527.0/1226.5;
                                    }
                                } else {
                                    return 358.0/1537.2;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_queue <= 1.00447046757f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 4607.0f ) {
                                return 605.0/1597.7;
                            } else {
                                return 875.0/1470.6;
                            }
                        } else {
                            return 1462.0/1139.8;
                        }
                    }
                }
            } else {
                if ( cl->stats.antecedents_glue_long_reds_var <= 6.93933820724f ) {
                    if ( cl->stats.glue_rel_long <= 0.696186423302f ) {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                            if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                                if ( cl->size() <= 10.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.29740011692f ) {
                                        return 399.0/1325.4;
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0505704917014f ) {
                                            return 429.0/1345.6;
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 0.517879962921f ) {
                                                return 305.0/1480.7;
                                            } else {
                                                return 331.0/1422.2;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->size() <= 17.5f ) {
                                        return 551.0/1387.9;
                                    } else {
                                        return 556.0/1065.1;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 25.5f ) {
                                    return 280.0/2039.5;
                                } else {
                                    return 298.0/1506.9;
                                }
                            }
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                                if ( cl->size() <= 7.5f ) {
                                    return 389.0/2666.9;
                                } else {
                                    return 612.0/2921.1;
                                }
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 5.5f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 8.5f ) {
                                        return 285.0/2432.9;
                                    } else {
                                        return 118.0/1932.6;
                                    }
                                } else {
                                    return 136.0/3574.7;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.dump_number <= 2.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 4568.0f ) {
                                return 800.0/1888.2;
                            } else {
                                return 1054.0/1480.7;
                            }
                        } else {
                            if ( cl->stats.size_rel <= 0.300548195839f ) {
                                return 305.0/1547.3;
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 7078.5f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.226813584566f ) {
                                        return 331.0/1779.3;
                                    } else {
                                        return 407.0/1533.2;
                                    }
                                } else {
                                    return 550.0/1488.8;
                                }
                            }
                        }
                    }
                } else {
                    if ( rdb0_last_touched_diff <= 10906.0f ) {
                        return 507.0/1166.0;
                    } else {
                        if ( cl->stats.dump_number <= 1.5f ) {
                            return 1484.0/532.6;
                        } else {
                            return 940.0/1220.5;
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                if ( cl->size() <= 14.5f ) {
                    if ( cl->stats.antec_num_total_lits_rel <= 0.72189116478f ) {
                        if ( rdb0_last_touched_diff <= 60360.0f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 21820.5f ) {
                                if ( cl->size() <= 10.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.35967373848f ) {
                                        return 666.0/1908.4;
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 8.5f ) {
                                            return 639.0/2713.3;
                                        } else {
                                            return 602.0/1831.7;
                                        }
                                    }
                                } else {
                                    return 869.0/2073.8;
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 19.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.284337937832f ) {
                                        return 516.0/1131.7;
                                    } else {
                                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                            return 519.0/1898.3;
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 33839.0f ) {
                                                return 535.0/1642.1;
                                            } else {
                                                return 501.0/1043.0;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 11.5f ) {
                                        return 1058.0/1740.9;
                                    } else {
                                        if ( cl->stats.dump_number <= 5.5f ) {
                                            return 739.0/1242.7;
                                        } else {
                                            if ( cl->stats.glue <= 6.5f ) {
                                                return 694.0/1843.8;
                                            } else {
                                                return 607.0/1244.7;
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.glue <= 6.5f ) {
                                if ( cl->stats.dump_number <= 33.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.414336204529f ) {
                                        return 643.0/893.7;
                                    } else {
                                        return 724.0/1234.6;
                                    }
                                } else {
                                    return 501.0/1190.2;
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 27.5f ) {
                                    return 676.0/782.7;
                                } else {
                                    return 669.0/710.1;
                                }
                            }
                        }
                    } else {
                        return 1309.0/1402.0;
                    }
                } else {
                    if ( cl->stats.num_overlap_literals <= 103.5f ) {
                        if ( cl->stats.size_rel <= 0.911794781685f ) {
                            if ( rdb0_last_touched_diff <= 54835.0f ) {
                                if ( cl->stats.dump_number <= 6.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.184058174491f ) {
                                        return 831.0/1115.6;
                                    } else {
                                        if ( cl->stats.glue <= 10.5f ) {
                                            return 632.0/806.9;
                                        } else {
                                            return 716.0/621.3;
                                        }
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 29122.0f ) {
                                        return 515.0/1218.5;
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0855153799057f ) {
                                            return 710.0/1139.8;
                                        } else {
                                            return 785.0/1404.1;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 74.5f ) {
                                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                        return 699.0/903.8;
                                    } else {
                                        return 1274.0/1190.2;
                                    }
                                } else {
                                    return 1240.0/730.3;
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.923906087875f ) {
                                if ( rdb0_last_touched_diff <= 51472.5f ) {
                                    return 1098.0/1460.5;
                                } else {
                                    return 933.0/597.1;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 30.5f ) {
                                    return 769.0/548.7;
                                } else {
                                    return 1633.0/675.8;
                                }
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 68037.0f ) {
                            if ( cl->stats.dump_number <= 6.5f ) {
                                if ( cl->stats.num_overlap_literals <= 382.5f ) {
                                    if ( cl->stats.glue <= 13.5f ) {
                                        return 757.0/637.5;
                                    } else {
                                        return 1466.0/389.3;
                                    }
                                } else {
                                    return 1282.0/296.5;
                                }
                            } else {
                                return 894.0/1299.2;
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.993756353855f ) {
                                return 892.0/427.7;
                            } else {
                                return 1481.0/201.7;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.num_overlap_literals <= 21.5f ) {
                    if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                        if ( cl->size() <= 12.5f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                                if ( cl->stats.glue_rel_long <= 0.320231199265f ) {
                                    return 408.0/1906.4;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 39257.0f ) {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.115911990404f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0547262653708f ) {
                                                return 269.0/1509.0;
                                            } else {
                                                return 237.0/1870.1;
                                            }
                                        } else {
                                            return 293.0/1577.5;
                                        }
                                    } else {
                                        return 327.0/1476.7;
                                    }
                                }
                            } else {
                                return 731.0/2368.3;
                            }
                        } else {
                            if ( cl->size() <= 21.5f ) {
                                return 623.0/2227.1;
                            } else {
                                return 571.0/1636.0;
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_long <= 0.830201387405f ) {
                            if ( cl->stats.num_overlap_literals_rel <= 0.0395814329386f ) {
                                if ( cl->stats.used_for_uip_creation <= 12.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.0619877316058f ) {
                                        return 333.0/1724.8;
                                    } else {
                                        return 344.0/2900.9;
                                    }
                                } else {
                                    return 142.0/2136.3;
                                }
                            } else {
                                if ( cl->stats.dump_number <= 15.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.562337875366f ) {
                                        return 303.0/2951.3;
                                    } else {
                                        return 148.0/1773.2;
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 24.5f ) {
                                        return 244.0/3389.1;
                                    } else {
                                        return 169.0/1833.7;
                                    }
                                }
                            }
                        } else {
                            return 256.0/2039.5;
                        }
                    }
                } else {
                    if ( cl->stats.size_rel <= 1.31676721573f ) {
                        if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                            if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                return 419.0/1678.4;
                            } else {
                                return 248.0/2092.0;
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 20585.5f ) {
                                return 292.0/1690.5;
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.216656059027f ) {
                                    return 488.0/1303.2;
                                } else {
                                    if ( rdb0_last_touched_diff <= 3938.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.8153424263f ) {
                                            return 298.0/1823.7;
                                        } else {
                                            return 292.0/1500.9;
                                        }
                                    } else {
                                        return 680.0/1797.4;
                                    }
                                }
                            }
                        }
                    } else {
                        return 486.0/1283.0;
                    }
                }
            }
        }
    } else {
        if ( cl->stats.glue <= 8.5f ) {
            if ( cl->stats.rdb1_last_touched_diff <= 38897.0f ) {
                if ( cl->size() <= 12.5f ) {
                    if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 8444.5f ) {
                            if ( cl->stats.glue_rel_long <= 0.544517874718f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 2439.5f ) {
                                    return 237.0/1797.4;
                                } else {
                                    return 392.0/2168.6;
                                }
                            } else {
                                return 536.0/1870.1;
                            }
                        } else {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.215771913528f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 17.5f ) {
                                    return 636.0/2124.2;
                                } else {
                                    return 521.0/1383.9;
                                }
                            } else {
                                return 499.0/1315.3;
                            }
                        }
                    } else {
                        if ( cl->stats.num_overlap_literals <= 38.5f ) {
                            if ( cl->size() <= 7.5f ) {
                                if ( cl->stats.dump_number <= 12.5f ) {
                                    if ( cl->stats.num_overlap_literals <= 9.5f ) {
                                        if ( cl->stats.glue_rel_long <= 0.430627316236f ) {
                                            return 567.0/1803.5;
                                        } else {
                                            return 406.0/1603.8;
                                        }
                                    } else {
                                        return 502.0/1359.7;
                                    }
                                } else {
                                    return 531.0/1172.1;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 7.5f ) {
                                    return 529.0/1456.5;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 25106.0f ) {
                                        return 630.0/1343.5;
                                    } else {
                                        return 598.0/930.0;
                                    }
                                }
                            }
                        } else {
                            return 694.0/909.8;
                        }
                    }
                } else {
                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.00353755615652f ) {
                        if ( rdb0_last_touched_diff <= 20341.0f ) {
                            return 427.0/1476.7;
                        } else {
                            return 871.0/1567.5;
                        }
                    } else {
                        if ( cl->stats.size_rel <= 0.923691809177f ) {
                            if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                return 1012.0/1551.3;
                            } else {
                                return 1169.0/1305.2;
                            }
                        } else {
                            return 1041.0/778.7;
                        }
                    }
                }
            } else {
                if ( cl->stats.antec_num_total_lits_rel <= 0.254514694214f ) {
                    if ( cl->stats.rdb1_act_ranking_top_10 <= 4.5f ) {
                        if ( rdb0_last_touched_diff <= 100383.5f ) {
                            if ( cl->stats.glue_rel_long <= 0.708366990089f ) {
                                if ( cl->size() <= 9.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0678051486611f ) {
                                        return 852.0/1307.2;
                                    } else {
                                        if ( cl->stats.size_rel <= 0.309985935688f ) {
                                            return 869.0/1494.8;
                                        } else {
                                            return 498.0/1049.0;
                                        }
                                    }
                                } else {
                                    return 1185.0/1248.7;
                                }
                            } else {
                                return 842.0/1040.9;
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 218536.5f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.154246836901f ) {
                                    if ( cl->stats.num_overlap_literals <= 5.5f ) {
                                        return 719.0/869.5;
                                    } else {
                                        return 959.0/865.4;
                                    }
                                } else {
                                    return 802.0/603.2;
                                }
                            } else {
                                return 1045.0/689.9;
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 122839.5f ) {
                            if ( cl->stats.glue_rel_long <= 0.542888760567f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.0381944440305f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.0977771580219f ) {
                                        return 665.0/837.2;
                                    } else {
                                        if ( rdb0_last_touched_diff <= 89402.0f ) {
                                            return 565.0/970.3;
                                        } else {
                                            return 606.0/875.5;
                                        }
                                    }
                                } else {
                                    return 694.0/651.6;
                                }
                            } else {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 5.5f ) {
                                    return 699.0/728.3;
                                } else {
                                    return 1300.0/893.7;
                                }
                            }
                        } else {
                            if ( cl->size() <= 10.5f ) {
                                if ( cl->stats.dump_number <= 21.5f ) {
                                    return 1399.0/827.1;
                                } else {
                                    if ( cl->size() <= 4.5f ) {
                                        return 946.0/998.6;
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.70216524601f ) {
                                            if ( cl->stats.dump_number <= 30.5f ) {
                                                return 729.0/712.1;
                                            } else {
                                                if ( cl->stats.dump_number <= 64.5f ) {
                                                    if ( cl->stats.num_overlap_literals_rel <= 0.0560099333525f ) {
                                                        return 854.0/508.4;
                                                    } else {
                                                        return 799.0/536.6;
                                                    }
                                                } else {
                                                    return 967.0/817.0;
                                                }
                                            }
                                        } else {
                                            return 805.0/498.3;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.536911010742f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.493184447289f ) {
                                        return 958.0/546.7;
                                    } else {
                                        return 1159.0/528.5;
                                    }
                                } else {
                                    return 1419.0/490.2;
                                }
                            }
                        }
                    }
                } else {
                    if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                        if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                            if ( cl->stats.size_rel <= 0.607055902481f ) {
                                return 819.0/901.7;
                            } else {
                                return 826.0/552.7;
                            }
                        } else {
                            if ( cl->stats.size_rel <= 0.39828363061f ) {
                                return 966.0/1065.1;
                            } else {
                                if ( rdb0_last_touched_diff <= 189098.0f ) {
                                    if ( cl->size() <= 14.5f ) {
                                        return 1200.0/1081.3;
                                    } else {
                                        return 1549.0/851.3;
                                    }
                                } else {
                                    return 899.0/280.4;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.dump_number <= 9.5f ) {
                            return 1131.0/774.7;
                        } else {
                            if ( cl->size() <= 11.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.294186472893f ) {
                                    return 839.0/419.6;
                                } else {
                                    return 1382.0/1022.8;
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 130014.5f ) {
                                    return 842.0/468.0;
                                } else {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 8.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.431954801083f ) {
                                            return 1013.0/419.6;
                                        } else {
                                            return 1363.0/347.0;
                                        }
                                    } else {
                                        return 1802.0/399.4;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.glue_rel_queue <= 0.88316488266f ) {
                if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                    if ( cl->stats.rdb1_last_touched_diff <= 28664.0f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 6411.0f ) {
                            return 463.0/1335.5;
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 65.5f ) {
                                return 539.0/1105.5;
                            } else {
                                return 706.0/764.6;
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 94686.5f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 61.5f ) {
                                return 675.0/815.0;
                            } else {
                                return 850.0/564.9;
                            }
                        } else {
                            return 944.0/439.8;
                        }
                    }
                } else {
                    if ( rdb0_last_touched_diff <= 63546.0f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 151.5f ) {
                            if ( cl->stats.dump_number <= 6.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.173749998212f ) {
                                    return 862.0/1563.4;
                                } else {
                                    return 1374.0/1143.8;
                                }
                            } else {
                                return 804.0/1377.8;
                            }
                        } else {
                            return 1583.0/796.8;
                        }
                    } else {
                        if ( cl->stats.glue_rel_queue <= 0.495106965303f ) {
                            if ( rdb0_last_touched_diff <= 152697.0f ) {
                                return 901.0/907.8;
                            } else {
                                return 1359.0/657.6;
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals_rel <= 0.0651236474514f ) {
                                if ( rdb0_last_touched_diff <= 157997.5f ) {
                                    return 1183.0/928.0;
                                } else {
                                    return 1645.0/619.3;
                                }
                            } else {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 12.4320831299f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 178589.0f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 154.5f ) {
                                            if ( rdb0_act_ranking_top_10 <= 7.5f ) {
                                                if ( cl->stats.num_total_lits_antecedents <= 62.5f ) {
                                                    return 1432.0/843.2;
                                                } else {
                                                    return 1154.0/591.1;
                                                }
                                            } else {
                                                return 1251.0/526.5;
                                            }
                                        } else {
                                            return 1202.0/462.0;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.727352559566f ) {
                                            if ( cl->stats.glue_rel_queue <= 0.659754991531f ) {
                                                return 940.0/349.0;
                                            } else {
                                                return 864.0/347.0;
                                            }
                                        } else {
                                            if ( rdb0_last_touched_diff <= 379411.5f ) {
                                                if ( cl->stats.dump_number <= 26.5f ) {
                                                    return 978.0/215.9;
                                                } else {
                                                    return 1010.0/324.8;
                                                }
                                            } else {
                                                return 955.0/179.5;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 105.5f ) {
                                        return 1772.0/532.6;
                                    } else {
                                        return 1175.0/250.1;
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                    if ( cl->stats.rdb1_last_touched_diff <= 44316.0f ) {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.253861486912f ) {
                            if ( cl->stats.dump_number <= 4.5f ) {
                                return 1352.0/1053.0;
                            } else {
                                return 715.0/1129.7;
                            }
                        } else {
                            if ( cl->stats.dump_number <= 5.5f ) {
                                if ( cl->stats.glue <= 14.5f ) {
                                    if ( cl->stats.num_overlap_literals <= 58.5f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 63.5f ) {
                                            return 904.0/472.1;
                                        } else {
                                            return 891.0/357.1;
                                        }
                                    } else {
                                        if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                            return 1698.0/597.1;
                                        } else {
                                            return 1133.0/254.2;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 1.42901396751f ) {
                                        if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.781105041504f ) {
                                                return 1723.0/405.5;
                                            } else {
                                                if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                                    return 1080.0/161.4;
                                                } else {
                                                    return 1562.0/197.7;
                                                }
                                            }
                                        } else {
                                            return 1449.0/371.2;
                                        }
                                    } else {
                                        if ( cl->stats.glue <= 35.5f ) {
                                            return 1879.0/228.0;
                                        } else {
                                            return 1325.0/72.6;
                                        }
                                    }
                                }
                            } else {
                                return 1263.0/1270.9;
                            }
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 1.10335600376f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 133192.5f ) {
                                if ( cl->stats.dump_number <= 14.5f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.348607718945f ) {
                                        if ( cl->stats.num_overlap_literals <= 12.5f ) {
                                            return 770.0/476.1;
                                        } else {
                                            return 1043.0/417.6;
                                        }
                                    } else {
                                        return 1250.0/351.0;
                                    }
                                } else {
                                    return 706.0/774.7;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 28.5f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0653114393353f ) {
                                        return 1083.0/282.4;
                                    } else {
                                        return 1581.0/552.7;
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.736161887646f ) {
                                        return 1254.0/298.6;
                                    } else {
                                        return 1107.0/173.5;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 1.12003588676f ) {
                                if ( cl->stats.glue <= 14.5f ) {
                                    if ( rdb0_last_touched_diff <= 123265.0f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 71921.5f ) {
                                            return 1723.0/437.8;
                                        } else {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 8.24845695496f ) {
                                                return 834.0/361.1;
                                            } else {
                                                return 932.0/308.7;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_act_ranking_top_10 <= 8.5f ) {
                                            if ( cl->stats.rdb1_act_ranking_top_10 <= 6.5f ) {
                                                if ( rdb0_last_touched_diff <= 253290.5f ) {
                                                    if ( cl->stats.num_total_lits_antecedents <= 107.5f ) {
                                                        return 941.0/278.4;
                                                    } else {
                                                        return 1101.0/221.9;
                                                    }
                                                } else {
                                                    return 1143.0/125.1;
                                                }
                                            } else {
                                                return 1406.0/431.7;
                                            }
                                        } else {
                                            return 1664.0/288.5;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 1.90683698654f ) {
                                        if ( cl->stats.dump_number <= 28.5f ) {
                                            if ( cl->stats.num_overlap_literals <= 158.5f ) {
                                                if ( cl->stats.glue_rel_queue <= 1.04083395004f ) {
                                                    if ( cl->stats.antecedents_glue_long_reds_var <= 26.2474479675f ) {
                                                        return 1030.0/234.0;
                                                    } else {
                                                        return 1068.0/177.5;
                                                    }
                                                } else {
                                                    return 1359.0/193.7;
                                                }
                                            } else {
                                                if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                                    return 1777.0/117.0;
                                                } else {
                                                    return 989.0/161.4;
                                                }
                                            }
                                        } else {
                                            return 1565.0/401.4;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 1.03521800041f ) {
                                            return 1029.0/98.8;
                                        } else {
                                            return 1030.0/52.5;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 33.5f ) {
                                    if ( cl->stats.size_rel <= 0.947180628777f ) {
                                        return 971.0/316.7;
                                    } else {
                                        return 1571.0/254.2;
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 1.29489302635f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.229962483048f ) {
                                            return 1343.0/88.8;
                                        } else {
                                            if ( cl->stats.dump_number <= 34.5f ) {
                                                if ( cl->stats.glue <= 12.5f ) {
                                                    return 1523.0/351.0;
                                                } else {
                                                    if ( cl->stats.num_total_lits_antecedents <= 114.5f ) {
                                                        return 1096.0/185.6;
                                                    } else {
                                                        if ( cl->stats.num_antecedents_rel <= 1.28154790401f ) {
                                                            if ( cl->size() <= 39.5f ) {
                                                                return 1581.0/139.2;
                                                            } else {
                                                                return 1740.0/242.1;
                                                            }
                                                        } else {
                                                            if ( cl->stats.num_overlap_literals_rel <= 2.96006822586f ) {
                                                                return 1658.0/108.9;
                                                            } else {
                                                                return 1006.0/125.1;
                                                            }
                                                        }
                                                    }
                                                }
                                            } else {
                                                return 1329.0/336.9;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.glue <= 21.5f ) {
                                            if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                                if ( cl->stats.num_overlap_literals <= 107.5f ) {
                                                    return 1654.0/195.7;
                                                } else {
                                                    return 1733.0/276.4;
                                                }
                                            } else {
                                                return 1273.0/92.8;
                                            }
                                        } else {
                                            if ( cl->stats.antec_num_total_lits_rel <= 1.38373267651f ) {
                                                if ( rdb0_last_touched_diff <= 125118.0f ) {
                                                    return 974.0/117.0;
                                                } else {
                                                    return 1207.0/58.5;
                                                }
                                            } else {
                                                if ( cl->stats.glue_rel_long <= 1.44141721725f ) {
                                                    return 992.0/58.5;
                                                } else {
                                                    return 1752.0/44.4;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    return 498.0/1083.3;
                }
            }
        }
    }
}

static double estimator_should_keep_short_conf3_cluster0_9(
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
        if ( rdb0_last_touched_diff <= 10018.5f ) {
            if ( cl->stats.antecedents_glue_long_reds_var <= 0.0749944448471f ) {
                if ( cl->stats.rdb1_last_touched_diff <= 8041.5f ) {
                    if ( cl->stats.glue_rel_long <= 0.422638952732f ) {
                        if ( cl->stats.antec_num_total_lits_rel <= 0.00417792890221f ) {
                            return 63.0/3114.7;
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0591837465763f ) {
                                        return 359.0/2364.3;
                                    } else {
                                        if ( rdb0_last_touched_diff <= 2668.0f ) {
                                            return 152.0/1851.9;
                                        } else {
                                            return 329.0/2408.7;
                                        }
                                    }
                                } else {
                                    if ( cl->size() <= 9.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0620287694037f ) {
                                            return 276.0/3100.6;
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.154309913516f ) {
                                                return 70.0/1940.7;
                                            } else {
                                                return 119.0/2392.5;
                                            }
                                        }
                                    } else {
                                        return 170.0/1849.9;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.177860558033f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 9.5f ) {
                                        return 137.0/2332.0;
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 19.5f ) {
                                            return 158.0/3405.2;
                                        } else {
                                            return 48.0/2299.7;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0889975428581f ) {
                                        if ( cl->stats.used_for_uip_creation <= 15.5f ) {
                                            if ( cl->stats.num_total_lits_antecedents <= 12.5f ) {
                                                if ( rdb0_last_touched_diff <= 1374.0f ) {
                                                    return 68.0/2467.2;
                                                } else {
                                                    return 122.0/2961.4;
                                                }
                                            } else {
                                                if ( cl->stats.glue_rel_queue <= 0.297555804253f ) {
                                                    return 103.0/2198.9;
                                                } else {
                                                    return 146.0/2376.4;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 9.5f ) {
                                                return 107.0/2572.1;
                                            } else {
                                                if ( cl->stats.num_antecedents_rel <= 0.114468857646f ) {
                                                    if ( cl->stats.rdb1_last_touched_diff <= 150.5f ) {
                                                        return 55.0/2245.3;
                                                    } else {
                                                        if ( cl->stats.glue_rel_long <= 0.261850774288f ) {
                                                            return 16.0/1995.1;
                                                        } else {
                                                            return 65.0/3441.6;
                                                        }
                                                    }
                                                } else {
                                                    return 63.0/2023.4;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.298400402069f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 1177.0f ) {
                                                if ( cl->stats.glue_rel_long <= 0.306104719639f ) {
                                                    return 48.0/2011.3;
                                                } else {
                                                    return 39.0/3566.6;
                                                }
                                            } else {
                                                if ( cl->stats.used_for_uip_creation <= 10.5f ) {
                                                    return 96.0/2390.5;
                                                } else {
                                                    return 59.0/2925.1;
                                                }
                                            }
                                        } else {
                                            return 106.0/2894.9;
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 4.5f ) {
                            if ( cl->stats.num_overlap_literals_rel <= 0.0371094867587f ) {
                                if ( cl->stats.num_overlap_literals <= 4.5f ) {
                                    return 326.0/3481.9;
                                } else {
                                    return 387.0/2249.3;
                                }
                            } else {
                                if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 2246.0f ) {
                                        return 181.0/1730.9;
                                    } else {
                                        return 338.0/2515.6;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                        return 287.0/3098.6;
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 9.5f ) {
                                            return 112.0/2741.5;
                                        } else {
                                            return 139.0/1972.9;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.0990379750729f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 672.5f ) {
                                        return 139.0/3443.6;
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.0879174470901f ) {
                                            return 239.0/3536.4;
                                        } else {
                                            return 105.0/1936.6;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 9.5f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 25.5f ) {
                                            return 154.0/3893.4;
                                        } else {
                                            if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                                return 180.0/2096.0;
                                            } else {
                                                return 147.0/2535.8;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.735931277275f ) {
                                            return 26.0/3112.7;
                                        } else {
                                            return 31.0/1975.0;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.563101530075f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 651.5f ) {
                                        return 44.0/2862.6;
                                    } else {
                                        return 97.0/3366.9;
                                    }
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 25.5f ) {
                                        return 181.0/3538.4;
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.144705116749f ) {
                                            return 60.0/2269.5;
                                        } else {
                                            return 29.0/2588.2;
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_last_touched_diff <= 35211.5f ) {
                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.0650093704462f ) {
                                if ( cl->stats.glue_rel_queue <= 0.422355532646f ) {
                                    if ( rdb0_last_touched_diff <= 1336.0f ) {
                                        return 181.0/2152.5;
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.0689146220684f ) {
                                            return 272.0/1553.3;
                                        } else {
                                            return 246.0/1626.0;
                                        }
                                    }
                                } else {
                                    return 410.0/2721.4;
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.883463144302f ) {
                                    if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                        if ( cl->stats.size_rel <= 0.289258301258f ) {
                                            return 441.0/2372.4;
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 23.5f ) {
                                                return 278.0/2443.0;
                                            } else {
                                                return 270.0/1648.2;
                                            }
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 1941.0f ) {
                                            if ( cl->stats.num_overlap_literals <= 5.5f ) {
                                                return 83.0/2420.8;
                                            } else {
                                                return 188.0/3766.3;
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 0.429966032505f ) {
                                                return 204.0/2130.3;
                                            } else {
                                                return 306.0/2493.4;
                                            }
                                        }
                                    }
                                } else {
                                    return 286.0/1845.8;
                                }
                            }
                        } else {
                            return 461.0/2489.4;
                        }
                    } else {
                        if ( cl->stats.num_overlap_literals <= 13.5f ) {
                            if ( cl->stats.num_overlap_literals_rel <= 0.0236966088414f ) {
                                return 451.0/1825.7;
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.543139219284f ) {
                                    if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                        return 390.0/1688.5;
                                    } else {
                                        return 184.0/1960.8;
                                    }
                                } else {
                                    return 412.0/2009.3;
                                }
                            }
                        } else {
                            return 473.0/1908.4;
                        }
                    }
                }
            } else {
                if ( rdb0_last_touched_diff <= 2873.5f ) {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 48.5f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                    return 406.0/3094.6;
                                } else {
                                    return 178.0/3197.5;
                                }
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                    return 316.0/1577.5;
                                } else {
                                    return 235.0/3159.1;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                                if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                    return 697.0/2324.0;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 22389.0f ) {
                                        return 191.0/2128.3;
                                    } else {
                                        return 287.0/2291.7;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.729379296303f ) {
                                    return 189.0/2057.7;
                                } else {
                                    return 273.0/1952.8;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 7.5f ) {
                            if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 2668.5f ) {
                                    return 198.0/1654.2;
                                } else {
                                    return 176.0/1702.6;
                                }
                            } else {
                                if ( cl->size() <= 10.5f ) {
                                    return 90.0/2576.1;
                                } else {
                                    if ( cl->stats.glue <= 9.5f ) {
                                        return 140.0/2134.3;
                                    } else {
                                        return 94.0/2085.9;
                                    }
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 927.5f ) {
                                if ( rdb0_last_touched_diff <= 94.5f ) {
                                    return 51.0/3397.2;
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 50.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.650123298168f ) {
                                            if ( rdb0_last_touched_diff <= 414.5f ) {
                                                return 56.0/2122.2;
                                            } else {
                                                return 43.0/1985.0;
                                            }
                                        } else {
                                            return 85.0/1922.5;
                                        }
                                    } else {
                                        return 47.0/2483.3;
                                    }
                                }
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 7.5f ) {
                                    return 156.0/2277.6;
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.556689560413f ) {
                                        return 60.0/2868.6;
                                    } else {
                                        return 88.0/2790.0;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 18235.5f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 156.5f ) {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.819878995419f ) {
                                        if ( cl->stats.glue_rel_long <= 0.628861606121f ) {
                                            return 604.0/3030.0;
                                        } else {
                                            return 302.0/1813.6;
                                        }
                                    } else {
                                        return 409.0/1585.6;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.651987671852f ) {
                                        return 319.0/2783.9;
                                    } else {
                                        return 285.0/1940.7;
                                    }
                                }
                            } else {
                                return 494.0/1749.0;
                            }
                        } else {
                            if ( cl->stats.glue <= 8.5f ) {
                                if ( cl->size() <= 11.5f ) {
                                    return 429.0/1734.9;
                                } else {
                                    return 462.0/1105.5;
                                }
                            } else {
                                return 1017.0/1926.5;
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                            if ( cl->stats.used_for_uip_creation <= 6.5f ) {
                                if ( cl->size() <= 13.5f ) {
                                    return 302.0/1678.4;
                                } else {
                                    return 453.0/1684.5;
                                }
                            } else {
                                return 268.0/1968.9;
                            }
                        } else {
                            if ( cl->stats.glue <= 8.5f ) {
                                if ( cl->stats.used_for_uip_creation <= 7.5f ) {
                                    if ( cl->size() <= 12.5f ) {
                                        if ( rdb0_last_touched_diff <= 5338.5f ) {
                                            return 141.0/1862.0;
                                        } else {
                                            return 161.0/1734.9;
                                        }
                                    } else {
                                        return 199.0/1621.9;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 1129.5f ) {
                                        return 114.0/1870.1;
                                    } else {
                                        return 186.0/2217.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 5.5f ) {
                                    return 388.0/2154.5;
                                } else {
                                    return 320.0/3187.4;
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.glue <= 9.5f ) {
                if ( rdb0_last_touched_diff <= 32872.5f ) {
                    if ( cl->stats.num_overlap_literals <= 11.5f ) {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                            if ( cl->stats.glue_rel_queue <= 0.222469687462f ) {
                                return 589.0/1636.0;
                            } else {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.375f ) {
                                    if ( cl->stats.size_rel <= 0.5022534132f ) {
                                        if ( cl->stats.size_rel <= 0.262901306152f ) {
                                            if ( cl->stats.num_overlap_literals <= 4.5f ) {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.0517842620611f ) {
                                                    return 338.0/1634.0;
                                                } else {
                                                    return 369.0/1408.1;
                                                }
                                            } else {
                                                if ( cl->stats.glue_rel_queue <= 0.50276529789f ) {
                                                    return 709.0/1999.2;
                                                } else {
                                                    return 385.0/1325.4;
                                                }
                                            }
                                        } else {
                                            if ( cl->size() <= 6.5f ) {
                                                return 317.0/1815.6;
                                            } else {
                                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                                    return 641.0/2455.1;
                                                } else {
                                                    return 478.0/1648.2;
                                                }
                                            }
                                        }
                                    } else {
                                        return 711.0/2116.2;
                                    }
                                } else {
                                    return 899.0/2299.7;
                                }
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 19.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.167591392994f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0356820747256f ) {
                                        if ( rdb0_last_touched_diff <= 13483.5f ) {
                                            return 488.0/2681.0;
                                        } else {
                                            return 481.0/2346.1;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 3494.0f ) {
                                            return 271.0/1835.8;
                                        } else {
                                            return 325.0/1785.3;
                                        }
                                    }
                                } else {
                                    return 368.0/2838.4;
                                }
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.104434363544f ) {
                                    return 392.0/1279.0;
                                } else {
                                    if ( cl->stats.dump_number <= 10.5f ) {
                                        return 356.0/1496.9;
                                    } else {
                                        return 315.0/1785.3;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                            if ( cl->size() <= 12.5f ) {
                                if ( cl->stats.glue_rel_long <= 0.738229036331f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 9719.5f ) {
                                        if ( rdb0_last_touched_diff <= 11610.0f ) {
                                            return 249.0/1583.6;
                                        } else {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.802777767181f ) {
                                                if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                                    return 527.0/1638.1;
                                                } else {
                                                    return 325.0/1597.7;
                                                }
                                            } else {
                                                return 348.0/1878.1;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.165882796049f ) {
                                            return 428.0/1194.3;
                                        } else {
                                            return 476.0/1492.8;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.470358073711f ) {
                                        return 561.0/2029.4;
                                    } else {
                                        return 493.0/1113.6;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                                    if ( cl->stats.glue <= 7.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.203987270594f ) {
                                            return 761.0/1539.2;
                                        } else {
                                            return 872.0/2166.6;
                                        }
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.470305234194f ) {
                                            return 964.0/1583.6;
                                        } else {
                                            return 602.0/1125.7;
                                        }
                                    }
                                } else {
                                    return 562.0/2241.2;
                                }
                            }
                        } else {
                            if ( cl->stats.size_rel <= 0.45786011219f ) {
                                return 500.0/1024.8;
                            } else {
                                return 539.0/1016.7;
                            }
                        }
                    }
                } else {
                    if ( cl->size() <= 9.5f ) {
                        if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                            if ( cl->stats.size_rel <= 0.0784592777491f ) {
                                return 317.0/1448.4;
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 44766.5f ) {
                                    if ( cl->stats.size_rel <= 0.21642126143f ) {
                                        return 480.0/1248.7;
                                    } else {
                                        return 548.0/1734.9;
                                    }
                                } else {
                                    return 564.0/1077.2;
                                }
                            }
                        } else {
                            if ( cl->stats.num_antecedents_rel <= 0.232407063246f ) {
                                if ( cl->stats.dump_number <= 37.5f ) {
                                    if ( cl->stats.dump_number <= 16.5f ) {
                                        return 806.0/1617.9;
                                    } else {
                                        return 607.0/964.3;
                                    }
                                } else {
                                    return 628.0/1392.0;
                                }
                            } else {
                                return 835.0/1337.5;
                            }
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 52.5f ) {
                            if ( rdb0_last_touched_diff <= 59964.5f ) {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 28.5f ) {
                                        return 511.0/1121.6;
                                    } else {
                                        return 676.0/1192.2;
                                    }
                                } else {
                                    return 1132.0/1736.9;
                                }
                            } else {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                    return 657.0/930.0;
                                } else {
                                    return 1147.0/1038.9;
                                }
                            }
                        } else {
                            if ( cl->stats.dump_number <= 9.5f ) {
                                if ( cl->stats.dump_number <= 5.5f ) {
                                    return 813.0/710.1;
                                } else {
                                    return 766.0/603.2;
                                }
                            } else {
                                if ( cl->size() <= 21.5f ) {
                                    return 714.0/938.1;
                                } else {
                                    return 697.0/811.0;
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.num_overlap_literals <= 71.5f ) {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 41301.0f ) {
                            if ( cl->stats.glue_rel_queue <= 0.91198015213f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 45.5f ) {
                                    if ( cl->stats.size_rel <= 0.534684598446f ) {
                                        return 435.0/1186.2;
                                    } else {
                                        return 550.0/996.6;
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 32128.0f ) {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 1.01999998093f ) {
                                            return 531.0/1008.7;
                                        } else {
                                            return 852.0/1206.4;
                                        }
                                    } else {
                                        return 830.0/984.5;
                                    }
                                }
                            } else {
                                if ( cl->stats.dump_number <= 4.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.453899532557f ) {
                                        return 1355.0/903.8;
                                    } else {
                                        return 849.0/363.1;
                                    }
                                } else {
                                    return 980.0/1932.6;
                                }
                            }
                        } else {
                            if ( cl->stats.size_rel <= 0.950781166553f ) {
                                if ( cl->size() <= 25.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.806916713715f ) {
                                        return 661.0/788.8;
                                    } else {
                                        return 763.0/621.3;
                                    }
                                } else {
                                    return 937.0/587.0;
                                }
                            } else {
                                return 1268.0/502.3;
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 6309.5f ) {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 4.5f ) {
                                return 722.0/1989.1;
                            } else {
                                return 504.0/2275.5;
                            }
                        } else {
                            return 518.0/1135.8;
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_last_touched_diff <= 17429.0f ) {
                        if ( cl->stats.glue_rel_queue <= 1.00132656097f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 220.5f ) {
                                return 700.0/1277.0;
                            } else {
                                return 1098.0/1264.9;
                            }
                        } else {
                            if ( cl->stats.dump_number <= 1.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 343.5f ) {
                                    return 930.0/429.7;
                                } else {
                                    return 968.0/189.6;
                                }
                            } else {
                                return 1387.0/1224.5;
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_long <= 1.01705384254f ) {
                            if ( cl->stats.dump_number <= 10.5f ) {
                                if ( cl->stats.size_rel <= 0.80635535717f ) {
                                    return 1084.0/548.7;
                                } else {
                                    return 1112.0/724.2;
                                }
                            } else {
                                return 799.0/746.4;
                            }
                        } else {
                            if ( cl->stats.glue <= 18.5f ) {
                                return 1638.0/615.3;
                            } else {
                                if ( cl->stats.num_overlap_literals <= 396.5f ) {
                                    return 1367.0/302.6;
                                } else {
                                    return 1162.0/84.7;
                                }
                            }
                        }
                    }
                }
            }
        }
    } else {
        if ( cl->stats.glue_rel_long <= 0.848673701286f ) {
            if ( rdb0_last_touched_diff <= 84518.5f ) {
                if ( cl->stats.glue_rel_queue <= 0.600432753563f ) {
                    if ( rdb0_last_touched_diff <= 29838.5f ) {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.0413223132491f ) {
                            if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                if ( cl->stats.glue_rel_long <= 0.350101143122f ) {
                                    return 512.0/2913.0;
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 21.5f ) {
                                        return 325.0/1549.3;
                                    } else {
                                        return 437.0/1402.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                    if ( cl->size() <= 7.5f ) {
                                        return 390.0/1484.7;
                                    } else {
                                        return 513.0/1198.3;
                                    }
                                } else {
                                    return 316.0/1494.8;
                                }
                            }
                        } else {
                            if ( cl->stats.glue <= 7.5f ) {
                                if ( cl->stats.size_rel <= 0.329601407051f ) {
                                    return 489.0/1430.3;
                                } else {
                                    return 497.0/1244.7;
                                }
                            } else {
                                return 598.0/972.3;
                            }
                        }
                    } else {
                        if ( cl->stats.num_overlap_literals <= 15.5f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.375f ) {
                                if ( cl->stats.size_rel <= 0.526120185852f ) {
                                    if ( cl->stats.dump_number <= 13.5f ) {
                                        if ( cl->stats.glue_rel_long <= 0.432397842407f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 48429.5f ) {
                                                if ( cl->stats.rdb1_last_touched_diff <= 30830.0f ) {
                                                    return 445.0/1125.7;
                                                } else {
                                                    return 524.0/1103.5;
                                                }
                                            } else {
                                                return 589.0/911.8;
                                            }
                                        } else {
                                            return 784.0/1827.7;
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0315874107182f ) {
                                            return 784.0/1170.0;
                                        } else {
                                            return 999.0/1970.9;
                                        }
                                    }
                                } else {
                                    return 614.0/839.2;
                                }
                            } else {
                                return 837.0/1226.5;
                            }
                        } else {
                            if ( cl->size() <= 10.5f ) {
                                return 724.0/1365.7;
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.177773535252f ) {
                                    return 756.0/546.7;
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.45801320672f ) {
                                        return 646.0/817.0;
                                    } else {
                                        return 934.0/932.0;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.254999995232f ) {
                            if ( rdb0_last_touched_diff <= 46021.0f ) {
                                if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                    if ( cl->stats.num_overlap_literals <= 11.5f ) {
                                        return 748.0/1934.6;
                                    } else {
                                        return 718.0/1222.5;
                                    }
                                } else {
                                    return 720.0/1155.9;
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.241629749537f ) {
                                    if ( cl->stats.glue_rel_long <= 0.687799572945f ) {
                                        return 651.0/958.2;
                                    } else {
                                        return 628.0/827.1;
                                    }
                                } else {
                                    return 917.0/857.4;
                                }
                            }
                        } else {
                            if ( cl->stats.glue <= 10.5f ) {
                                if ( rdb0_last_touched_diff <= 39027.5f ) {
                                    if ( cl->stats.num_overlap_literals <= 43.5f ) {
                                        return 916.0/1504.9;
                                    } else {
                                        return 875.0/714.1;
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 92.5f ) {
                                        if ( cl->size() <= 12.5f ) {
                                            return 708.0/1020.8;
                                        } else {
                                            return 1219.0/934.0;
                                        }
                                    } else {
                                        return 1185.0/581.0;
                                    }
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                    return 1271.0/817.0;
                                } else {
                                    return 1381.0/423.6;
                                }
                            }
                        }
                    } else {
                        return 754.0/2394.6;
                    }
                }
            } else {
                if ( cl->stats.num_overlap_literals_rel <= 0.109441488981f ) {
                    if ( cl->stats.antec_num_total_lits_rel <= 0.142572328448f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 132056.5f ) {
                            if ( rdb0_last_touched_diff <= 105497.0f ) {
                                if ( cl->stats.dump_number <= 13.5f ) {
                                    return 782.0/819.0;
                                } else {
                                    return 739.0/1127.7;
                                }
                            } else {
                                if ( cl->size() <= 9.5f ) {
                                    return 1286.0/1545.3;
                                } else {
                                    return 988.0/748.4;
                                }
                            }
                        } else {
                            if ( cl->size() <= 11.5f ) {
                                if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                    return 880.0/905.8;
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0559526085854f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.102208130062f ) {
                                            if ( rdb0_last_touched_diff <= 266630.5f ) {
                                                return 945.0/911.8;
                                            } else {
                                                return 898.0/522.5;
                                            }
                                        } else {
                                            return 1469.0/879.6;
                                        }
                                    } else {
                                        return 1040.0/1000.6;
                                    }
                                }
                            } else {
                                if ( cl->stats.size_rel <= 0.405265688896f ) {
                                    if ( rdb0_last_touched_diff <= 286858.0f ) {
                                        return 1102.0/643.5;
                                    } else {
                                        return 877.0/294.5;
                                    }
                                } else {
                                    return 1079.0/282.4;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_long <= 0.425833582878f ) {
                            return 748.0/609.2;
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 22.5f ) {
                                return 1277.0/932.0;
                            } else {
                                if ( rdb0_last_touched_diff <= 285332.5f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.0694444477558f ) {
                                        return 950.0/655.6;
                                    } else {
                                        return 1261.0/615.3;
                                    }
                                } else {
                                    return 943.0/252.2;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_total_lits_antecedents <= 23.5f ) {
                        return 1104.0/1073.2;
                    } else {
                        if ( cl->size() <= 11.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 273901.5f ) {
                                if ( cl->stats.glue_rel_queue <= 0.732409119606f ) {
                                    if ( rdb0_last_touched_diff <= 113307.5f ) {
                                        return 628.0/748.4;
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 24.5f ) {
                                            return 1071.0/754.5;
                                        } else {
                                            return 720.0/821.1;
                                        }
                                    }
                                } else {
                                    return 948.0/587.0;
                                }
                            } else {
                                return 1202.0/522.5;
                            }
                        } else {
                            if ( cl->stats.size_rel <= 0.857584118843f ) {
                                if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                    if ( cl->stats.num_overlap_literals <= 43.5f ) {
                                        return 1201.0/796.8;
                                    } else {
                                        return 1060.0/490.2;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.496791601181f ) {
                                        return 1371.0/831.1;
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 136346.5f ) {
                                            if ( cl->stats.num_antecedents_rel <= 0.397756814957f ) {
                                                return 885.0/435.7;
                                            } else {
                                                return 822.0/453.9;
                                            }
                                        } else {
                                            if ( cl->stats.dump_number <= 19.5f ) {
                                                return 897.0/185.6;
                                            } else {
                                                if ( cl->stats.antecedents_glue_long_reds_var <= 1.21604943275f ) {
                                                    return 1460.0/651.6;
                                                } else {
                                                    if ( cl->stats.num_antecedents_rel <= 0.380506366491f ) {
                                                        return 995.0/377.2;
                                                    } else {
                                                        return 977.0/316.7;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                    if ( rdb0_last_touched_diff <= 175784.5f ) {
                                        return 1379.0/679.8;
                                    } else {
                                        return 1138.0/353.0;
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 377684.5f ) {
                                        if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                            if ( cl->stats.glue_rel_queue <= 0.771737337112f ) {
                                                return 1229.0/353.0;
                                            } else {
                                                return 865.0/326.8;
                                            }
                                        } else {
                                            return 1457.0/318.7;
                                        }
                                    } else {
                                        return 1050.0/145.2;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.rdb1_last_touched_diff <= 45151.0f ) {
                if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                    if ( cl->stats.antec_num_total_lits_rel <= 0.665098965168f ) {
                        if ( cl->size() <= 14.5f ) {
                            if ( cl->stats.num_overlap_literals <= 22.5f ) {
                                if ( cl->stats.size_rel <= 0.448821306229f ) {
                                    return 526.0/1049.0;
                                } else {
                                    return 544.0/972.3;
                                }
                            } else {
                                return 719.0/700.0;
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 201.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.319444447756f ) {
                                    if ( cl->stats.dump_number <= 4.5f ) {
                                        return 874.0/863.4;
                                    } else {
                                        return 693.0/1049.0;
                                    }
                                } else {
                                    if ( cl->size() <= 19.5f ) {
                                        return 785.0/548.7;
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.997779965401f ) {
                                            return 791.0/522.5;
                                        } else {
                                            if ( cl->stats.dump_number <= 3.5f ) {
                                                return 1067.0/280.4;
                                            } else {
                                                return 946.0/397.4;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 170.5f ) {
                                    return 920.0/240.1;
                                } else {
                                    return 1033.0/177.5;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_long <= 1.10268628597f ) {
                            if ( cl->stats.dump_number <= 4.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 1.25982451439f ) {
                                    return 1701.0/641.5;
                                } else {
                                    return 1138.0/302.6;
                                }
                            } else {
                                return 779.0/581.0;
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 16.6437492371f ) {
                                if ( cl->stats.glue <= 12.5f ) {
                                    return 1025.0/534.6;
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 270.5f ) {
                                        return 901.0/332.9;
                                    } else {
                                        return 1047.0/171.5;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue <= 21.5f ) {
                                    return 1113.0/187.6;
                                } else {
                                    if ( cl->size() <= 77.5f ) {
                                        return 1091.0/171.5;
                                    } else {
                                        return 1769.0/119.0;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.260904490948f ) {
                        return 392.0/1365.7;
                    } else {
                        return 903.0/1000.6;
                    }
                }
            } else {
                if ( cl->size() <= 13.5f ) {
                    if ( cl->stats.glue <= 9.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 114384.5f ) {
                            if ( cl->stats.num_antecedents_rel <= 0.349011600018f ) {
                                return 902.0/1127.7;
                            } else {
                                return 764.0/492.2;
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 36.5f ) {
                                return 1124.0/599.1;
                            } else {
                                return 997.0/407.5;
                            }
                        }
                    } else {
                        if ( cl->stats.antec_num_total_lits_rel <= 0.685017704964f ) {
                            return 1009.0/377.2;
                        } else {
                            return 1567.0/286.5;
                        }
                    }
                } else {
                    if ( cl->stats.antec_num_total_lits_rel <= 0.677586019039f ) {
                        if ( cl->stats.num_overlap_literals <= 22.5f ) {
                            if ( cl->stats.num_overlap_literals_rel <= 0.180314391851f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0304929092526f ) {
                                    return 1380.0/447.8;
                                } else {
                                    if ( cl->stats.glue <= 22.5f ) {
                                        if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                            return 1379.0/649.6;
                                        } else {
                                            if ( cl->stats.dump_number <= 27.5f ) {
                                                return 1215.0/407.5;
                                            } else {
                                                return 914.0/276.4;
                                            }
                                        }
                                    } else {
                                        return 811.0/488.2;
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 147388.5f ) {
                                    return 889.0/365.1;
                                } else {
                                    return 1234.0/177.5;
                                }
                            }
                        } else {
                            if ( cl->stats.glue <= 11.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 159047.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 1.00643420219f ) {
                                        return 914.0/419.6;
                                    } else {
                                        return 929.0/312.7;
                                    }
                                } else {
                                    return 1645.0/270.3;
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.185329690576f ) {
                                    return 1157.0/143.2;
                                } else {
                                    if ( cl->stats.dump_number <= 45.5f ) {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.507777810097f ) {
                                            return 1070.0/447.8;
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 1.26509332657f ) {
                                                if ( cl->stats.dump_number <= 14.5f ) {
                                                    if ( cl->stats.antecedents_glue_long_reds_var <= 17.5103302002f ) {
                                                        return 1262.0/191.6;
                                                    } else {
                                                        return 1670.0/165.4;
                                                    }
                                                } else {
                                                    if ( cl->stats.num_total_lits_antecedents <= 105.5f ) {
                                                        return 1365.0/276.4;
                                                    } else {
                                                        return 1685.0/288.5;
                                                    }
                                                }
                                            } else {
                                                if ( rdb0_last_touched_diff <= 136910.0f ) {
                                                    return 1217.0/143.2;
                                                } else {
                                                    return 1204.0/66.6;
                                                }
                                            }
                                        }
                                    } else {
                                        return 1154.0/324.8;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.num_overlap_literals <= 347.5f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 27.9937496185f ) {
                                if ( cl->stats.glue_rel_queue <= 1.03390407562f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 7.91319465637f ) {
                                        if ( rdb0_last_touched_diff <= 185273.0f ) {
                                            return 1351.0/351.0;
                                        } else {
                                            return 1221.0/197.7;
                                        }
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 13.6311340332f ) {
                                            return 927.0/308.7;
                                        } else {
                                            return 937.0/217.9;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue <= 10.5f ) {
                                        return 919.0/248.1;
                                    } else {
                                        if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                            if ( cl->stats.dump_number <= 40.5f ) {
                                                if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                                    if ( cl->stats.num_overlap_literals_rel <= 1.70815813541f ) {
                                                        if ( cl->stats.num_antecedents_rel <= 0.744934916496f ) {
                                                            return 1005.0/161.4;
                                                        } else {
                                                            if ( cl->stats.num_overlap_literals <= 102.5f ) {
                                                                return 1134.0/135.2;
                                                            } else {
                                                                return 1038.0/82.7;
                                                            }
                                                        }
                                                    } else {
                                                        return 950.0/161.4;
                                                    }
                                                } else {
                                                    if ( rdb0_last_touched_diff <= 162424.0f ) {
                                                        return 923.0/232.0;
                                                    } else {
                                                        return 955.0/161.4;
                                                    }
                                                }
                                            } else {
                                                return 953.0/260.2;
                                            }
                                        } else {
                                            if ( cl->stats.size_rel <= 1.31177341938f ) {
                                                return 1173.0/129.1;
                                            } else {
                                                return 1446.0/98.8;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 5.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.792502343655f ) {
                                        return 991.0/60.5;
                                    } else {
                                        return 974.0/98.8;
                                    }
                                } else {
                                    if ( cl->stats.glue <= 21.5f ) {
                                        return 1239.0/205.8;
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 1.24969482422f ) {
                                            return 1012.0/169.5;
                                        } else {
                                            return 1331.0/88.8;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals <= 1722.0f ) {
                                if ( cl->stats.glue <= 20.5f ) {
                                    if ( cl->stats.num_overlap_literals <= 527.5f ) {
                                        return 969.0/137.2;
                                    } else {
                                        return 1069.0/181.6;
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 1.1527159214f ) {
                                        if ( cl->stats.num_antecedents_rel <= 1.58915221691f ) {
                                            return 1043.0/151.3;
                                        } else {
                                            return 996.0/84.7;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_act_ranking_top_10 <= 6.5f ) {
                                            return 1443.0/50.4;
                                        } else {
                                            return 1383.0/102.9;
                                        }
                                    }
                                }
                            } else {
                                return 1729.0/74.6;
                            }
                        }
                    }
                }
            }
        }
    }
}

static bool should_keep_short_conf3_cluster0(
    const CMSat::Clause* cl
    , const uint64_t sumConflicts
    , const uint32_t rdb0_last_touched_diff
    , const uint32_t rdb0_act_ranking
    , const uint32_t rdb0_act_ranking_top_10
) {
    int votes = 0;
    votes += estimator_should_keep_short_conf3_cluster0_0(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_short_conf3_cluster0_1(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_short_conf3_cluster0_2(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_short_conf3_cluster0_3(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_short_conf3_cluster0_4(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_short_conf3_cluster0_5(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_short_conf3_cluster0_6(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_short_conf3_cluster0_7(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_short_conf3_cluster0_8(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_short_conf3_cluster0_9(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    return votes >= 5;
}
}
