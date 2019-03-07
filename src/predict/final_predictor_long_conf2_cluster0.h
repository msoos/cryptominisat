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

static double estimator_should_keep_long_conf2_cluster0_0(
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
        if ( cl->size() <= 11.5f ) {
            if ( cl->stats.sum_uip1_used <= 21.5f ) {
                if ( cl->stats.sum_uip1_used <= 3.5f ) {
                    if ( cl->size() <= 7.5f ) {
                        if ( cl->size() <= 4.5f ) {
                            if ( cl->stats.dump_number <= 3.5f ) {
                                return 102.0/350.2;
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                    return 102.0/212.1;
                                } else {
                                    return 144.0/224.1;
                                }
                            }
                        } else {
                            if ( cl->stats.dump_number <= 13.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 28104.0f ) {
                                    if ( cl->stats.size_rel <= 0.168479889631f ) {
                                        return 98.0/256.1;
                                    } else {
                                        return 200.0/408.2;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 4.5f ) {
                                        return 214.0/342.2;
                                    } else {
                                        return 160.0/168.1;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.492509543896f ) {
                                    return 149.0/76.0;
                                } else {
                                    return 170.0/104.1;
                                }
                            }
                        }
                    } else {
                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 30.5f ) {
                                return 120.0/248.1;
                            } else {
                                return 136.0/196.1;
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 83081.0f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 49.5f ) {
                                    if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.19292268157f ) {
                                            return 159.0/254.1;
                                        } else {
                                            return 170.0/156.1;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.678217411041f ) {
                                            return 132.0/170.1;
                                        } else {
                                            return 176.0/120.1;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 4.8046875f ) {
                                        return 194.0/122.1;
                                    } else {
                                        return 228.0/52.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.142370909452f ) {
                                    if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                        return 211.0/136.1;
                                    } else {
                                        return 219.0/96.1;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.705567657948f ) {
                                        return 208.0/102.1;
                                    } else {
                                        if ( cl->stats.size_rel <= 0.491853147745f ) {
                                            return 260.0/98.1;
                                        } else {
                                            return 178.0/26.0;
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.dump_number <= 13.5f ) {
                        if ( rdb0_last_touched_diff <= 42997.5f ) {
                            if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0173128731549f ) {
                                    return 98.0/226.1;
                                } else {
                                    if ( cl->stats.size_rel <= 0.126831352711f ) {
                                        return 57.0/408.2;
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.796360552311f ) {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 4.5f ) {
                                                if ( cl->stats.sum_uip1_used <= 14.5f ) {
                                                    if ( cl->stats.glue_rel_long <= 0.51523411274f ) {
                                                        return 165.0/400.2;
                                                    } else {
                                                        return 82.0/356.2;
                                                    }
                                                } else {
                                                    return 63.0/356.2;
                                                }
                                            } else {
                                                return 49.0/322.2;
                                            }
                                        } else {
                                            return 86.0/246.1;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.129768326879f ) {
                                    return 105.0/278.2;
                                } else {
                                    return 105.0/202.1;
                                }
                            }
                        } else {
                            if ( cl->stats.dump_number <= 8.5f ) {
                                if ( cl->stats.glue <= 5.5f ) {
                                    return 103.0/278.2;
                                } else {
                                    return 129.0/222.1;
                                }
                            } else {
                                if ( cl->size() <= 7.5f ) {
                                    return 117.0/222.1;
                                } else {
                                    return 188.0/176.1;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.size_rel <= 0.455573916435f ) {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 788104.0f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 108362.5f ) {
                                    return 199.0/280.2;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 47489.0f ) {
                                        return 139.0/190.1;
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 15.5f ) {
                                            return 143.0/128.1;
                                        } else {
                                            if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                                return 161.0/84.0;
                                            } else {
                                                return 159.0/102.1;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                    return 165.0/424.2;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 47430.0f ) {
                                        return 143.0/192.1;
                                    } else {
                                        if ( rdb0_last_touched_diff <= 129914.0f ) {
                                            return 167.0/116.1;
                                        } else {
                                            return 156.0/144.1;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 98600.5f ) {
                                return 159.0/130.1;
                            } else {
                                return 242.0/104.1;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.sum_uip1_used <= 66.5f ) {
                    if ( cl->stats.rdb1_last_touched_diff <= 36650.0f ) {
                        if ( cl->stats.glue_rel_queue <= 0.690151333809f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.115434668958f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0396921038628f ) {
                                        if ( rdb0_last_touched_diff <= 16200.5f ) {
                                            return 52.0/396.2;
                                        } else {
                                            return 81.0/260.1;
                                        }
                                    } else {
                                        return 52.0/390.2;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.116412095726f ) {
                                        return 114.0/350.2;
                                    } else {
                                        return 75.0/392.2;
                                    }
                                }
                            } else {
                                if ( cl->stats.dump_number <= 15.5f ) {
                                    return 90.0/520.3;
                                } else {
                                    return 124.0/244.1;
                                }
                            }
                        } else {
                            return 120.0/402.2;
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 192035.0f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.0546875f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 2087416.5f ) {
                                    return 152.0/534.3;
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.480001211166f ) {
                                        return 145.0/178.1;
                                    } else {
                                        return 94.0/216.1;
                                    }
                                }
                            } else {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.988165676594f ) {
                                    return 139.0/200.1;
                                } else {
                                    return 107.0/244.1;
                                }
                            }
                        } else {
                            return 177.0/260.1;
                        }
                    }
                } else {
                    if ( cl->stats.dump_number <= 21.5f ) {
                        if ( cl->stats.num_antecedents_rel <= 0.0549114719033f ) {
                            return 76.0/486.3;
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 24941.5f ) {
                                if ( cl->stats.dump_number <= 7.5f ) {
                                    return 11.0/480.3;
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 124.5f ) {
                                        return 52.0/460.3;
                                    } else {
                                        return 13.0/514.3;
                                    }
                                }
                            } else {
                                return 70.0/588.3;
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 95426.5f ) {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 75455392.0f ) {
                                    if ( cl->stats.sum_uip1_used <= 160.5f ) {
                                        if ( cl->size() <= 6.5f ) {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 18272008.0f ) {
                                                return 89.0/328.2;
                                            } else {
                                                return 90.0/244.1;
                                            }
                                        } else {
                                            if ( cl->stats.dump_number <= 39.5f ) {
                                                return 63.0/302.2;
                                            } else {
                                                return 75.0/278.2;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.dump_number <= 39.5f ) {
                                            return 60.0/446.3;
                                        } else {
                                            return 45.0/386.2;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 0.177897512913f ) {
                                        return 55.0/388.2;
                                    } else {
                                        return 41.0/752.4;
                                    }
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.123043946922f ) {
                                    if ( cl->stats.sum_uip1_used <= 302.5f ) {
                                        return 41.0/322.2;
                                    } else {
                                        return 13.0/420.2;
                                    }
                                } else {
                                    return 40.0/350.2;
                                }
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals_rel <= 0.0248842611909f ) {
                                return 142.0/146.1;
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.508558034897f ) {
                                    return 113.0/464.3;
                                } else {
                                    return 95.0/230.1;
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                if ( cl->stats.glue_rel_queue <= 0.827986001968f ) {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 4.5f ) {
                        if ( cl->stats.sum_uip1_used <= 11.5f ) {
                            if ( cl->stats.size_rel <= 0.82321947813f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.322530865669f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 39.5f ) {
                                        return 126.0/158.1;
                                    } else {
                                        return 147.0/266.2;
                                    }
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                        if ( rdb0_last_touched_diff <= 27732.0f ) {
                                            return 228.0/266.2;
                                        } else {
                                            return 197.0/150.1;
                                        }
                                    } else {
                                        return 193.0/94.1;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 39.5f ) {
                                    return 153.0/94.1;
                                } else {
                                    return 252.0/72.0;
                                }
                            }
                        } else {
                            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.dump_number <= 13.5f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.104286983609f ) {
                                        return 97.0/328.2;
                                    } else {
                                        return 53.0/374.2;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.641756892204f ) {
                                        if ( cl->size() <= 19.5f ) {
                                            return 147.0/408.2;
                                        } else {
                                            return 93.0/340.2;
                                        }
                                    } else {
                                        return 117.0/240.1;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                                    return 131.0/236.1;
                                } else {
                                    return 106.0/240.1;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.sum_uip1_used <= 24.5f ) {
                            return 133.0/326.2;
                        } else {
                            if ( cl->stats.size_rel <= 0.506184220314f ) {
                                return 49.0/540.3;
                            } else {
                                return 66.0/372.2;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.sum_delta_confl_uip1_used <= 3173.0f ) {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 91.0f ) {
                            if ( cl->stats.glue_rel_long <= 0.871747434139f ) {
                                return 196.0/60.0;
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.227964803576f ) {
                                    return 221.0/68.0;
                                } else {
                                    if ( cl->stats.dump_number <= 2.5f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 138.0f ) {
                                            return 260.0/38.0;
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 486.5f ) {
                                                return 322.0/6.0;
                                            } else {
                                                return 202.0/20.0;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.glue <= 15.5f ) {
                                            return 203.0/2.0;
                                        } else {
                                            return 238.0/8.0;
                                        }
                                    }
                                }
                            }
                        } else {
                            return 275.0/132.1;
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 219.5f ) {
                            if ( cl->stats.dump_number <= 15.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 6434.5f ) {
                                    return 137.0/250.1;
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.966317594051f ) {
                                        return 130.0/184.1;
                                    } else {
                                        return 154.0/110.1;
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 4549660.5f ) {
                                    return 163.0/198.1;
                                } else {
                                    return 112.0/398.2;
                                }
                            }
                        } else {
                            return 210.0/164.1;
                        }
                    }
                }
            } else {
                if ( cl->stats.sum_delta_confl_uip1_used <= 733.5f ) {
                    if ( cl->stats.num_overlap_literals_rel <= 0.298042595387f ) {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.104070216417f ) {
                            if ( cl->stats.dump_number <= 2.5f ) {
                                return 160.0/176.1;
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 27.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 55202.0f ) {
                                        return 218.0/110.1;
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 205539.5f ) {
                                            if ( cl->stats.num_overlap_literals <= 7.5f ) {
                                                return 197.0/54.0;
                                            } else {
                                                return 304.0/62.0;
                                            }
                                        } else {
                                            return 330.0/102.1;
                                        }
                                    }
                                } else {
                                    return 214.0/152.1;
                                }
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 36.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 184315.0f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.146568149328f ) {
                                        return 181.0/114.1;
                                    } else {
                                        return 214.0/90.1;
                                    }
                                } else {
                                    return 199.0/58.0;
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.920376181602f ) {
                                    if ( rdb0_last_touched_diff <= 60669.0f ) {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 2.5f ) {
                                            return 220.0/70.0;
                                        } else {
                                            return 165.0/100.1;
                                        }
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 10.724445343f ) {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 4.01234579086f ) {
                                                if ( cl->size() <= 27.5f ) {
                                                    return 261.0/64.0;
                                                } else {
                                                    return 224.0/24.0;
                                                }
                                            } else {
                                                return 213.0/18.0;
                                            }
                                        } else {
                                            return 239.0/72.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 56657.5f ) {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 18.84375f ) {
                                            return 268.0/64.0;
                                        } else {
                                            return 286.0/20.0;
                                        }
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.0692662745714f ) {
                                            return 200.0/2.0;
                                        } else {
                                            if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                                if ( cl->stats.num_antecedents_rel <= 0.187344700098f ) {
                                                    return 196.0/20.0;
                                                } else {
                                                    return 323.0/6.0;
                                                }
                                            } else {
                                                if ( cl->stats.num_overlap_literals <= 27.5f ) {
                                                    return 204.0/16.0;
                                                } else {
                                                    return 233.0/32.0;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 6.06069660187f ) {
                            if ( rdb0_last_touched_diff <= 60240.5f ) {
                                if ( cl->stats.size_rel <= 0.80760383606f ) {
                                    return 307.0/142.1;
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                        return 257.0/28.0;
                                    } else {
                                        return 321.0/102.1;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 190.5f ) {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 5.5f ) {
                                        if ( cl->stats.size_rel <= 0.886034727097f ) {
                                            return 234.0/66.0;
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 0.97565805912f ) {
                                                return 197.0/24.0;
                                            } else {
                                                return 245.0/4.0;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 62.5f ) {
                                            return 242.0/12.0;
                                        } else {
                                            if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                                if ( cl->stats.antecedents_glue_long_reds_var <= 1.67105424404f ) {
                                                    return 247.0/144.1;
                                                } else {
                                                    return 247.0/82.0;
                                                }
                                            } else {
                                                return 367.0/58.0;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 10.5f ) {
                                        return 258.0/34.0;
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 1.91746377945f ) {
                                            return 378.0/14.0;
                                        } else {
                                            return 236.0/18.0;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 546669.5f ) {
                                if ( cl->stats.glue <= 17.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.811633527279f ) {
                                        return 324.0/96.1;
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 26.1108646393f ) {
                                            if ( cl->size() <= 16.5f ) {
                                                if ( cl->stats.rdb1_last_touched_diff <= 53055.0f ) {
                                                    return 184.0/56.0;
                                                } else {
                                                    return 339.0/38.0;
                                                }
                                            } else {
                                                if ( cl->stats.antecedents_glue_long_reds_var <= 7.41120910645f ) {
                                                    return 239.0/40.0;
                                                } else {
                                                    if ( cl->stats.num_total_lits_antecedents <= 76.5f ) {
                                                        return 184.0/30.0;
                                                    } else {
                                                        if ( cl->stats.glue_rel_long <= 1.13029682636f ) {
                                                            if ( cl->stats.glue_rel_queue <= 1.05983006954f ) {
                                                                if ( cl->stats.antec_num_total_lits_rel <= 0.895485043526f ) {
                                                                    return 209.0/8.0;
                                                                } else {
                                                                    return 262.0/20.0;
                                                                }
                                                            } else {
                                                                return 244.0/2.0;
                                                            }
                                                        } else {
                                                            if ( cl->stats.num_antecedents_rel <= 0.648888409138f ) {
                                                                return 223.0/6.0;
                                                            } else {
                                                                if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                                                    return 261.0/36.0;
                                                                } else {
                                                                    return 253.0/24.0;
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 115084.5f ) {
                                                return 242.0/66.0;
                                            } else {
                                                return 197.0/30.0;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 168.0f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.912913024426f ) {
                                            return 183.0/48.0;
                                        } else {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 386.361328125f ) {
                                                if ( cl->stats.num_antecedents_rel <= 0.502839207649f ) {
                                                    if ( cl->size() <= 155.5f ) {
                                                        if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                                            return 1;
                                                        } else {
                                                            return 223.0/6.0;
                                                        }
                                                    } else {
                                                        return 206.0/10.0;
                                                    }
                                                } else {
                                                    if ( cl->stats.num_overlap_literals <= 241.5f ) {
                                                        if ( cl->stats.rdb1_act_ranking_top_10 <= 5.5f ) {
                                                            if ( cl->stats.num_antecedents_rel <= 0.874210119247f ) {
                                                                return 213.0/22.0;
                                                            } else {
                                                                return 238.0/8.0;
                                                            }
                                                        } else {
                                                            if ( cl->stats.rdb1_act_ranking_top_10 <= 8.5f ) {
                                                                return 244.0/60.0;
                                                            } else {
                                                                return 215.0/12.0;
                                                            }
                                                        }
                                                    } else {
                                                        if ( cl->stats.size_rel <= 1.04932618141f ) {
                                                            if ( cl->stats.num_overlap_literals <= 434.5f ) {
                                                                return 221.0/26.0;
                                                            } else {
                                                                if ( cl->stats.size_rel <= 0.540335118771f ) {
                                                                    return 258.0/6.0;
                                                                } else {
                                                                    return 332.0/20.0;
                                                                }
                                                            }
                                                        } else {
                                                            if ( cl->stats.num_overlap_literals_rel <= 3.76626253128f ) {
                                                                if ( cl->stats.num_total_lits_antecedents <= 865.0f ) {
                                                                    if ( cl->size() <= 104.5f ) {
                                                                        return 209.0/2.0;
                                                                    } else {
                                                                        return 1;
                                                                    }
                                                                } else {
                                                                    return 287.0/10.0;
                                                                }
                                                            } else {
                                                                return 285.0/22.0;
                                                            }
                                                        }
                                                    }
                                                }
                                            } else {
                                                return 198.0/46.0;
                                            }
                                        }
                                    } else {
                                        return 189.0/44.0;
                                    }
                                }
                            } else {
                                return 251.0/92.1;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.glue_rel_queue <= 0.569710373878f ) {
                        if ( cl->stats.dump_number <= 14.5f ) {
                            if ( rdb0_last_touched_diff <= 59642.0f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 138065.0f ) {
                                    return 195.0/350.2;
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.389728188515f ) {
                                        return 44.0/340.2;
                                    } else {
                                        return 55.0/320.2;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0933083295822f ) {
                                    return 136.0/224.1;
                                } else {
                                    return 135.0/154.1;
                                }
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 76.5f ) {
                                if ( cl->stats.num_overlap_literals <= 12.5f ) {
                                    if ( rdb0_last_touched_diff <= 192536.0f ) {
                                        return 246.0/166.1;
                                    } else {
                                        return 195.0/46.0;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 20.5f ) {
                                        return 175.0/198.1;
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 187843.0f ) {
                                            return 249.0/178.1;
                                        } else {
                                            return 178.0/66.0;
                                        }
                                    }
                                }
                            } else {
                                return 170.0/420.2;
                            }
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 522.5f ) {
                            if ( cl->stats.sum_uip1_used <= 21.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 39289.5f ) {
                                    if ( cl->stats.glue <= 8.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 20444.5f ) {
                                            return 129.0/162.1;
                                        } else {
                                            return 145.0/132.1;
                                        }
                                    } else {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 116790.0f ) {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.807736456394f ) {
                                                return 107.0/174.1;
                                            } else {
                                                return 314.0/182.1;
                                            }
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 101.5f ) {
                                                return 185.0/96.1;
                                            } else {
                                                return 178.0/66.0;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.722819328308f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.633986234665f ) {
                                            if ( cl->size() <= 20.5f ) {
                                                return 173.0/90.1;
                                            } else {
                                                return 223.0/36.0;
                                            }
                                        } else {
                                            if ( cl->stats.size_rel <= 0.586414337158f ) {
                                                return 163.0/156.1;
                                            } else {
                                                return 350.0/140.1;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.dump_number <= 10.5f ) {
                                            if ( cl->stats.num_total_lits_antecedents <= 101.5f ) {
                                                return 227.0/132.1;
                                            } else {
                                                return 207.0/74.0;
                                            }
                                        } else {
                                            if ( cl->stats.dump_number <= 78.5f ) {
                                                if ( rdb0_last_touched_diff <= 182799.0f ) {
                                                    if ( cl->stats.antecedents_glue_long_reds_var <= 7.9921875f ) {
                                                        if ( cl->size() <= 46.5f ) {
                                                            if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                                                if ( cl->size() <= 19.5f ) {
                                                                    return 203.0/78.0;
                                                                } else {
                                                                    return 206.0/24.0;
                                                                }
                                                            } else {
                                                                return 217.0/108.1;
                                                            }
                                                        } else {
                                                            return 165.0/88.0;
                                                        }
                                                    } else {
                                                        if ( cl->stats.sum_delta_confl_uip1_used <= 120815.0f ) {
                                                            return 268.0/20.0;
                                                        } else {
                                                            return 192.0/48.0;
                                                        }
                                                    }
                                                } else {
                                                    if ( cl->stats.size_rel <= 0.57567512989f ) {
                                                        return 167.0/64.0;
                                                    } else {
                                                        if ( cl->stats.glue_rel_long <= 1.03901171684f ) {
                                                            if ( cl->stats.num_antecedents_rel <= 0.26871702075f ) {
                                                                return 188.0/44.0;
                                                            } else {
                                                                return 325.0/20.0;
                                                            }
                                                        } else {
                                                            return 223.0/54.0;
                                                        }
                                                    }
                                                }
                                            } else {
                                                return 208.0/78.0;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_uip1_used <= 76.5f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 4716025.0f ) {
                                        if ( rdb0_last_touched_diff <= 120561.0f ) {
                                            if ( cl->stats.num_antecedents_rel <= 0.141064926982f ) {
                                                return 124.0/174.1;
                                            } else {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.315390467644f ) {
                                                    return 82.0/234.1;
                                                } else {
                                                    return 105.0/238.1;
                                                }
                                            }
                                        } else {
                                            return 209.0/192.1;
                                        }
                                    } else {
                                        return 265.0/180.1;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.729233980179f ) {
                                        return 104.0/338.2;
                                    } else {
                                        if ( cl->stats.sum_uip1_used <= 160.5f ) {
                                            return 74.0/266.2;
                                        } else {
                                            return 38.0/330.2;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                return 293.0/30.0;
                            } else {
                                return 190.0/30.0;
                            }
                        }
                    }
                }
            }
        }
    } else {
        if ( rdb0_last_touched_diff <= 2400.5f ) {
            if ( cl->stats.rdb1_last_touched_diff <= 10920.0f ) {
                if ( cl->stats.sum_uip1_used <= 34.5f ) {
                    if ( cl->size() <= 12.5f ) {
                        if ( cl->stats.sum_uip1_used <= 7.5f ) {
                            return 60.0/292.2;
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                if ( rdb0_last_touched_diff <= 1029.0f ) {
                                    return 55.0/334.2;
                                } else {
                                    return 47.0/326.2;
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.160820156336f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0330212116241f ) {
                                        return 32.0/454.3;
                                    } else {
                                        return 24.0/662.4;
                                    }
                                } else {
                                    return 31.0/340.2;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 3.58251142502f ) {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 174365.5f ) {
                                return 75.0/296.2;
                            } else {
                                return 74.0/536.3;
                            }
                        } else {
                            return 99.0/216.1;
                        }
                    }
                } else {
                    if ( cl->stats.sum_uip1_used <= 236.5f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 27.5f ) {
                            if ( cl->stats.dump_number <= 22.5f ) {
                                if ( rdb0_last_touched_diff <= 81.0f ) {
                                    return 34.0/548.3;
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 87.5f ) {
                                        if ( cl->stats.dump_number <= 8.5f ) {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.0730823129416f ) {
                                                if ( cl->stats.sum_delta_confl_uip1_used <= 974432.5f ) {
                                                    return 9.0/412.2;
                                                } else {
                                                    return 21.0/432.2;
                                                }
                                            } else {
                                                if ( rdb0_last_touched_diff <= 833.5f ) {
                                                    return 4.0/426.2;
                                                } else {
                                                    return 14.0/410.2;
                                                }
                                            }
                                        } else {
                                            return 38.0/578.3;
                                        }
                                    } else {
                                        if ( cl->stats.size_rel <= 0.0853907987475f ) {
                                            return 17.0/612.3;
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 0.477897942066f ) {
                                                if ( cl->stats.rdb1_used_for_uip_creation <= 23.5f ) {
                                                    if ( cl->stats.num_antecedents_rel <= 0.122043676674f ) {
                                                        return 3.0/424.2;
                                                    } else {
                                                        return 1.0/454.3;
                                                    }
                                                } else {
                                                    return 6.0/388.2;
                                                }
                                            } else {
                                                return 9.0/564.3;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.dump_number <= 28.5f ) {
                                    return 42.0/354.2;
                                } else {
                                    if ( cl->stats.dump_number <= 39.5f ) {
                                        return 11.0/440.2;
                                    } else {
                                        return 45.0/388.2;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                return 96.0/572.3;
                            } else {
                                if ( cl->stats.dump_number <= 4.5f ) {
                                    if ( cl->stats.sum_uip1_used <= 66.5f ) {
                                        return 6.0/612.3;
                                    } else {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 1387780.5f ) {
                                            return 17.0/430.2;
                                        } else {
                                            return 10.0/544.3;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 0.238193631172f ) {
                                        return 24.0/782.4;
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 18.5f ) {
                                            return 60.0/580.3;
                                        } else {
                                            if ( cl->stats.sum_uip1_used <= 99.5f ) {
                                                return 24.0/334.2;
                                            } else {
                                                return 20.0/532.3;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.used_for_uip_creation <= 8.5f ) {
                            if ( cl->stats.num_antecedents_rel <= 0.0760792195797f ) {
                                return 17.0/400.2;
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 158607008.0f ) {
                                    if ( cl->stats.sum_uip1_used <= 346.5f ) {
                                        return 12.0/384.2;
                                    } else {
                                        return 11.0/428.2;
                                    }
                                } else {
                                    return 5.0/442.2;
                                }
                            }
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 280.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.0565449520946f ) {
                                    if ( cl->size() <= 5.5f ) {
                                        if ( cl->stats.size_rel <= 0.0511581301689f ) {
                                            return 1.0/462.3;
                                        } else {
                                            return 9.0/718.4;
                                        }
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 24.5f ) {
                                            return 22.0/432.2;
                                        } else {
                                            return 12.0/516.3;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 43.5f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 31.5f ) {
                                            if ( cl->stats.glue_rel_queue <= 0.488043069839f ) {
                                                if ( cl->stats.glue_rel_long <= 0.380812942982f ) {
                                                    if ( cl->stats.glue_rel_queue <= 0.320051193237f ) {
                                                        return 3.0/738.4;
                                                    } else {
                                                        return 6.0/412.2;
                                                    }
                                                } else {
                                                    return 0.0/500.3;
                                                }
                                            } else {
                                                return 9.0/506.3;
                                            }
                                        } else {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0694445222616f ) {
                                                return 17.0/366.2;
                                            } else {
                                                if ( cl->stats.glue <= 9.5f ) {
                                                    return 3.0/504.3;
                                                } else {
                                                    return 9.0/364.2;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.322670221329f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 139.5f ) {
                                                return 6.0/360.2;
                                            } else {
                                                return 1.0/454.3;
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 729.0f ) {
                                                if ( cl->stats.rdb1_used_for_uip_creation <= 75.5f ) {
                                                    return 1.0/408.2;
                                                } else {
                                                    return 0.0/794.4;
                                                }
                                            } else {
                                                return 4.0/420.2;
                                            }
                                        }
                                    }
                                }
                            } else {
                                return 16.0/450.3;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                    if ( cl->stats.sum_uip1_used <= 13.5f ) {
                        return 173.0/212.1;
                    } else {
                        return 96.0/384.2;
                    }
                } else {
                    if ( cl->stats.num_overlap_literals <= 20.5f ) {
                        if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                            if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                return 49.0/406.2;
                            } else {
                                return 31.0/708.4;
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 28.5f ) {
                                return 116.0/520.3;
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 6.5f ) {
                                    return 69.0/496.3;
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.384901940823f ) {
                                        return 14.0/382.2;
                                    } else {
                                        return 12.0/498.3;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 877031.0f ) {
                            return 95.0/284.2;
                        } else {
                            return 89.0/584.3;
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.sum_delta_confl_uip1_used <= 1062195.0f ) {
                if ( cl->stats.rdb1_last_touched_diff <= 27998.5f ) {
                    if ( cl->stats.num_overlap_literals_rel <= 0.130517214537f ) {
                        if ( cl->stats.sum_uip1_used <= 17.5f ) {
                            if ( rdb0_last_touched_diff <= 3793.0f ) {
                                return 86.0/460.3;
                            } else {
                                if ( cl->stats.size_rel <= 0.526825726032f ) {
                                    if ( cl->stats.sum_uip1_used <= 8.5f ) {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 29778.5f ) {
                                            return 75.0/324.2;
                                        } else {
                                            return 116.0/354.2;
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 6839.5f ) {
                                            return 57.0/448.3;
                                        } else {
                                            return 68.0/336.2;
                                        }
                                    }
                                } else {
                                    return 125.0/232.1;
                                }
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 50.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.033668294549f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 3707.0f ) {
                                        return 36.0/468.3;
                                    } else {
                                        return 51.0/318.2;
                                    }
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                        return 44.0/506.3;
                                    } else {
                                        return 29.0/698.4;
                                    }
                                }
                            } else {
                                return 66.0/292.2;
                            }
                        }
                    } else {
                        if ( cl->size() <= 11.5f ) {
                            if ( cl->stats.dump_number <= 3.5f ) {
                                return 78.0/574.3;
                            } else {
                                return 74.0/336.2;
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 2.27777767181f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 202890.0f ) {
                                    return 105.0/256.1;
                                } else {
                                    return 63.0/308.2;
                                }
                            } else {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 8.23611068726f ) {
                                    return 133.0/168.1;
                                } else {
                                    return 97.0/228.1;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->size() <= 11.5f ) {
                        return 192.0/362.2;
                    } else {
                        if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                            return 212.0/138.1;
                        } else {
                            return 146.0/158.1;
                        }
                    }
                }
            } else {
                if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                    if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 39.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 7937.5f ) {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 10.5f ) {
                                    if ( cl->stats.sum_uip1_used <= 42.5f ) {
                                        return 95.0/410.2;
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 5.5f ) {
                                            if ( cl->stats.dump_number <= 37.5f ) {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.0658852010965f ) {
                                                    return 18.0/382.2;
                                                } else {
                                                    return 13.0/470.3;
                                                }
                                            } else {
                                                return 32.0/400.2;
                                            }
                                        } else {
                                            if ( cl->stats.sum_uip1_used <= 121.0f ) {
                                                return 56.0/400.2;
                                            } else {
                                                return 24.0/514.3;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 5.5f ) {
                                        return 15.0/466.3;
                                    } else {
                                        return 29.0/392.2;
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 6418.5f ) {
                                    return 68.0/606.3;
                                } else {
                                    return 58.0/320.2;
                                }
                            }
                        } else {
                            if ( cl->stats.dump_number <= 18.5f ) {
                                return 74.0/588.3;
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.736513376236f ) {
                                    return 78.0/432.2;
                                } else {
                                    return 110.0/282.2;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 4.5f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.3671875f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.0734392851591f ) {
                                    return 51.0/544.3;
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0686728730798f ) {
                                        return 34.0/418.2;
                                    } else {
                                        return 22.0/592.3;
                                    }
                                }
                            } else {
                                return 57.0/288.2;
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 4229.5f ) {
                                if ( cl->stats.glue_rel_long <= 0.244739592075f ) {
                                    return 32.0/640.4;
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 36.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0589816048741f ) {
                                            if ( cl->stats.glue_rel_queue <= 0.390434443951f ) {
                                                return 9.0/630.4;
                                            } else {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.0576044507325f ) {
                                                    return 21.0/388.2;
                                                } else {
                                                    return 15.0/410.2;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 944.0f ) {
                                                return 2.0/460.3;
                                            } else {
                                                return 7.0/372.2;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.glue <= 8.5f ) {
                                            return 32.0/432.2;
                                        } else {
                                            return 25.0/732.4;
                                        }
                                    }
                                }
                            } else {
                                return 42.0/480.3;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                        if ( cl->stats.sum_uip1_used <= 44.5f ) {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 2130058.0f ) {
                                return 90.0/274.2;
                            } else {
                                return 184.0/260.1;
                            }
                        } else {
                            if ( cl->stats.dump_number <= 25.5f ) {
                                if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                    return 52.0/328.2;
                                } else {
                                    return 24.0/480.3;
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.464392870665f ) {
                                    return 49.0/334.2;
                                } else {
                                    return 63.0/274.2;
                                }
                            }
                        }
                    } else {
                        return 29.0/402.2;
                    }
                }
            }
        }
    }
}

static double estimator_should_keep_long_conf2_cluster0_1(
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
        if ( cl->stats.size_rel <= 0.498592495918f ) {
            if ( cl->stats.sum_uip1_used <= 19.5f ) {
                if ( cl->stats.antecedents_glue_long_reds_var <= 3.51890397072f ) {
                    if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                        if ( cl->stats.size_rel <= 0.0662589371204f ) {
                            if ( cl->stats.num_overlap_literals_rel <= 0.0176179539412f ) {
                                return 92.0/346.2;
                            } else {
                                return 54.0/406.2;
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.0135030867532f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 23557.0f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 528594.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0412585660815f ) {
                                            return 109.0/204.1;
                                        } else {
                                            if ( rdb0_last_touched_diff <= 15320.5f ) {
                                                if ( cl->stats.sum_uip1_used <= 8.5f ) {
                                                    if ( cl->stats.num_overlap_literals_rel <= 0.0727761983871f ) {
                                                        return 81.0/352.2;
                                                    } else {
                                                        return 78.0/270.2;
                                                    }
                                                } else {
                                                    return 41.0/336.2;
                                                }
                                            } else {
                                                if ( cl->stats.num_total_lits_antecedents <= 17.5f ) {
                                                    return 99.0/302.2;
                                                } else {
                                                    return 169.0/368.2;
                                                }
                                            }
                                        }
                                    } else {
                                        return 227.0/376.2;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                        return 166.0/368.2;
                                    } else {
                                        if ( cl->size() <= 8.5f ) {
                                            return 189.0/224.1;
                                        } else {
                                            return 165.0/130.1;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.glue <= 7.5f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0511434003711f ) {
                                        return 226.0/288.2;
                                    } else {
                                        if ( cl->size() <= 11.5f ) {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 53591.5f ) {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.223692506552f ) {
                                                    return 72.0/266.2;
                                                } else {
                                                    return 100.0/254.1;
                                                }
                                            } else {
                                                return 197.0/406.2;
                                            }
                                        } else {
                                            return 166.0/224.1;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 63.5f ) {
                                        return 218.0/282.2;
                                    } else {
                                        return 179.0/84.0;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 521827.0f ) {
                            if ( cl->stats.sum_uip1_used <= 6.5f ) {
                                return 140.0/586.3;
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.147946447134f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.0637693405151f ) {
                                        return 77.0/440.2;
                                    } else {
                                        if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                            return 36.0/398.2;
                                        } else {
                                            return 17.0/430.2;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 35.5f ) {
                                        return 64.0/602.3;
                                    } else {
                                        return 78.0/288.2;
                                    }
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 3016.0f ) {
                                return 72.0/278.2;
                            } else {
                                return 110.0/244.1;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.glue_rel_long <= 0.756684541702f ) {
                        if ( cl->size() <= 12.5f ) {
                            if ( cl->stats.num_overlap_literals <= 33.5f ) {
                                return 92.0/210.1;
                            } else {
                                return 65.0/282.2;
                            }
                        } else {
                            return 196.0/220.1;
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 11234.0f ) {
                            return 112.0/196.1;
                        } else {
                            if ( cl->stats.sum_uip1_used <= 1.5f ) {
                                return 308.0/70.0;
                            } else {
                                return 186.0/136.1;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                    if ( cl->stats.dump_number <= 17.5f ) {
                        if ( cl->stats.used_for_uip_creation <= 11.5f ) {
                            if ( rdb0_last_touched_diff <= 18303.0f ) {
                                if ( cl->size() <= 9.5f ) {
                                    if ( cl->stats.sum_uip1_used <= 32.5f ) {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 636920.5f ) {
                                            return 29.0/360.2;
                                        } else {
                                            return 66.0/352.2;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 11657.5f ) {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 3137562.0f ) {
                                                return 23.0/600.3;
                                            } else {
                                                return 25.0/398.2;
                                            }
                                        } else {
                                            return 58.0/630.4;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 34.5f ) {
                                        return 72.0/270.2;
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.108490005136f ) {
                                            return 50.0/288.2;
                                        } else {
                                            return 37.0/354.2;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.130927041173f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 1610917.5f ) {
                                        return 136.0/490.3;
                                    } else {
                                        return 53.0/416.2;
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 10.5f ) {
                                        return 67.0/418.2;
                                    } else {
                                        return 37.0/350.2;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 32.5f ) {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                                    return 29.0/666.4;
                                } else {
                                    return 44.0/456.3;
                                }
                            } else {
                                return 10.0/582.3;
                            }
                        }
                    } else {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 11163964.0f ) {
                            if ( rdb0_last_touched_diff <= 18656.0f ) {
                                if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                    if ( cl->stats.num_overlap_literals <= 4.5f ) {
                                        return 100.0/386.2;
                                    } else {
                                        if ( cl->stats.dump_number <= 28.5f ) {
                                            return 122.0/408.2;
                                        } else {
                                            return 99.0/214.1;
                                        }
                                    }
                                } else {
                                    return 81.0/560.3;
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 27.5f ) {
                                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                        return 98.0/302.2;
                                    } else {
                                        return 171.0/394.2;
                                    }
                                } else {
                                    return 166.0/260.1;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 30012.0f ) {
                                if ( rdb0_last_touched_diff <= 8342.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.499169439077f ) {
                                        if ( rdb0_last_touched_diff <= 1649.5f ) {
                                            if ( cl->stats.size_rel <= 0.207198470831f ) {
                                                return 18.0/414.2;
                                            } else {
                                                return 7.0/424.2;
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 11060.0f ) {
                                                if ( cl->stats.num_overlap_literals <= 4.5f ) {
                                                    return 16.0/372.2;
                                                } else {
                                                    return 24.0/392.2;
                                                }
                                            } else {
                                                return 54.0/490.3;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.sum_uip1_used <= 220.5f ) {
                                            return 69.0/384.2;
                                        } else {
                                            return 17.0/442.2;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 47201272.0f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.07472884655f ) {
                                            return 91.0/234.1;
                                        } else {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0662039965391f ) {
                                                return 101.0/428.2;
                                            } else {
                                                return 83.0/458.3;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.glue <= 5.5f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0468777120113f ) {
                                                return 18.0/486.3;
                                            } else {
                                                return 44.0/400.2;
                                            }
                                        } else {
                                            return 55.0/336.2;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.dump_number <= 78.5f ) {
                                    if ( rdb0_last_touched_diff <= 5861.0f ) {
                                        return 39.0/490.3;
                                    } else {
                                        return 119.0/608.3;
                                    }
                                } else {
                                    return 175.0/434.2;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                        if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                            if ( cl->size() <= 6.5f ) {
                                if ( cl->stats.glue_rel_long <= 0.280655145645f ) {
                                    return 56.0/492.3;
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.446662247181f ) {
                                        return 46.0/584.3;
                                    } else {
                                        return 28.0/468.3;
                                    }
                                }
                            } else {
                                if ( cl->stats.size_rel <= 0.261490762234f ) {
                                    return 114.0/546.3;
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.619699597359f ) {
                                        if ( cl->stats.dump_number <= 17.5f ) {
                                            return 41.0/354.2;
                                        } else {
                                            return 35.0/386.2;
                                        }
                                    } else {
                                        return 61.0/318.2;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 55.5f ) {
                                if ( cl->stats.dump_number <= 14.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 25.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0762139260769f ) {
                                            return 54.0/454.3;
                                        } else {
                                            return 22.0/382.2;
                                        }
                                    } else {
                                        return 63.0/380.2;
                                    }
                                } else {
                                    return 59.0/322.2;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 27.5f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0107033159584f ) {
                                        return 38.0/390.2;
                                    } else {
                                        if ( cl->stats.sum_uip1_used <= 249.5f ) {
                                            if ( cl->stats.glue_rel_long <= 0.29777520895f ) {
                                                return 37.0/400.2;
                                            } else {
                                                if ( cl->stats.glue_rel_queue <= 0.508647561073f ) {
                                                    if ( cl->stats.sum_delta_confl_uip1_used <= 10512529.0f ) {
                                                        return 4.0/480.3;
                                                    } else {
                                                        return 16.0/376.2;
                                                    }
                                                } else {
                                                    return 30.0/484.3;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 0.428878009319f ) {
                                                return 17.0/628.4;
                                            } else {
                                                return 3.0/420.2;
                                            }
                                        }
                                    }
                                } else {
                                    return 46.0/346.2;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 19.5f ) {
                            if ( cl->stats.used_for_uip_creation <= 8.5f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 31485066.0f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 1502.5f ) {
                                        return 21.0/788.4;
                                    } else {
                                        return 45.0/664.4;
                                    }
                                } else {
                                    return 5.0/666.4;
                                }
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 138510176.0f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0178232118487f ) {
                                        if ( rdb0_last_touched_diff <= 1065.0f ) {
                                            if ( cl->stats.glue_rel_long <= 0.303724169731f ) {
                                                return 2.0/626.4;
                                            } else {
                                                return 4.0/418.2;
                                            }
                                        } else {
                                            return 15.0/550.3;
                                        }
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.0461294613779f ) {
                                            return 42.0/618.3;
                                        } else {
                                            if ( cl->stats.used_for_uip_creation <= 35.5f ) {
                                                if ( cl->stats.num_total_lits_antecedents <= 8.5f ) {
                                                    return 3.0/414.2;
                                                } else {
                                                    if ( cl->stats.num_overlap_literals_rel <= 0.0232974663377f ) {
                                                        return 3.0/406.2;
                                                    } else {
                                                        if ( cl->stats.sum_uip1_used <= 45.5f ) {
                                                            return 27.0/412.2;
                                                        } else {
                                                            if ( cl->stats.dump_number <= 26.5f ) {
                                                                if ( cl->stats.used_for_uip_creation <= 24.5f ) {
                                                                    if ( cl->stats.sum_uip1_used <= 103.5f ) {
                                                                        return 10.0/550.3;
                                                                    } else {
                                                                        return 5.0/700.4;
                                                                    }
                                                                } else {
                                                                    return 11.0/404.2;
                                                                }
                                                            } else {
                                                                return 27.0/520.3;
                                                            }
                                                        }
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.sum_uip1_used <= 167.5f ) {
                                                    return 8.0/406.2;
                                                } else {
                                                    if ( cl->stats.glue_rel_long <= 0.377308011055f ) {
                                                        return 6.0/730.4;
                                                    } else {
                                                        return 0.0/440.2;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 14.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0168942399323f ) {
                                            return 10.0/412.2;
                                        } else {
                                            return 3.0/810.5;
                                        }
                                    } else {
                                        return 1.0/404.2;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 19.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0539438202977f ) {
                                    if ( cl->stats.glue <= 6.5f ) {
                                        if ( cl->stats.glue_rel_long <= 0.299111992121f ) {
                                            return 25.0/382.2;
                                        } else {
                                            return 14.0/474.3;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 14.5f ) {
                                            return 46.0/400.2;
                                        } else {
                                            return 22.0/380.2;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 111.5f ) {
                                        if ( rdb0_last_touched_diff <= 2320.0f ) {
                                            return 22.0/776.4;
                                        } else {
                                            return 43.0/494.3;
                                        }
                                    } else {
                                        if ( cl->stats.dump_number <= 31.5f ) {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 17535232.0f ) {
                                                return 8.0/446.3;
                                            } else {
                                                return 2.0/464.3;
                                            }
                                        } else {
                                            return 11.0/422.2;
                                        }
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 866.0f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 33.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 586.5f ) {
                                            return 24.0/568.3;
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 35.5f ) {
                                                return 8.0/396.2;
                                            } else {
                                                return 3.0/490.3;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 154.5f ) {
                                            return 12.0/776.4;
                                        } else {
                                            if ( cl->stats.dump_number <= 8.5f ) {
                                                return 0.0/536.3;
                                            } else {
                                                return 3.0/466.3;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->size() <= 7.5f ) {
                                        return 38.0/442.2;
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.527723670006f ) {
                                            if ( cl->stats.num_overlap_literals <= 14.5f ) {
                                                return 25.0/374.2;
                                            } else {
                                                return 18.0/396.2;
                                            }
                                        } else {
                                            return 14.0/644.4;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if ( rdb0_last_touched_diff <= 10103.5f ) {
                if ( cl->stats.sum_uip1_used <= 25.5f ) {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 156.0f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 22117.0f ) {
                                if ( cl->stats.dump_number <= 6.5f ) {
                                    return 139.0/454.3;
                                } else {
                                    return 120.0/200.1;
                                }
                            } else {
                                if ( cl->stats.size_rel <= 0.76009452343f ) {
                                    return 225.0/246.1;
                                } else {
                                    return 173.0/302.2;
                                }
                            }
                        } else {
                            return 212.0/164.1;
                        }
                    } else {
                        if ( cl->stats.sum_uip1_used <= 10.5f ) {
                            return 145.0/288.2;
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.669516563416f ) {
                                return 45.0/332.2;
                            } else {
                                return 104.0/382.2;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                            if ( cl->stats.size_rel <= 0.727208852768f ) {
                                return 94.0/530.3;
                            } else {
                                return 128.0/458.3;
                            }
                        } else {
                            return 45.0/602.3;
                        }
                    } else {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 19068540.0f ) {
                            if ( cl->stats.used_for_uip_creation <= 10.5f ) {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                    return 84.0/486.3;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 1441.5f ) {
                                        return 18.0/404.2;
                                    } else {
                                        return 37.0/416.2;
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 534.5f ) {
                                    return 14.0/688.4;
                                } else {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 4.5f ) {
                                        return 43.0/368.2;
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.214860320091f ) {
                                            return 36.0/410.2;
                                        } else {
                                            return 15.0/426.2;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 2.49489784241f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0404462069273f ) {
                                    return 21.0/382.2;
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 45.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.305621892214f ) {
                                            return 16.0/436.2;
                                        } else {
                                            return 9.0/492.3;
                                        }
                                    } else {
                                        return 1.0/442.2;
                                    }
                                }
                            } else {
                                return 33.0/416.2;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.sum_uip1_used <= 11.5f ) {
                    if ( cl->stats.sum_delta_confl_uip1_used <= 43.5f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 58.5f ) {
                            if ( cl->stats.size_rel <= 0.702946305275f ) {
                                return 155.0/200.1;
                            } else {
                                return 220.0/74.0;
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.887015938759f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.554536581039f ) {
                                    return 182.0/36.0;
                                } else {
                                    return 180.0/60.0;
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 28564.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 255.0f ) {
                                        if ( cl->stats.glue <= 14.5f ) {
                                            return 291.0/30.0;
                                        } else {
                                            return 202.0/52.0;
                                        }
                                    } else {
                                        return 338.0/14.0;
                                    }
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 2.5f ) {
                                        if ( cl->stats.size_rel <= 1.08052062988f ) {
                                            return 204.0/2.0;
                                        } else {
                                            return 255.0/2.0;
                                        }
                                    } else {
                                        return 310.0/20.0;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->size() <= 21.5f ) {
                            if ( cl->stats.dump_number <= 24.5f ) {
                                if ( cl->stats.size_rel <= 0.796882987022f ) {
                                    if ( rdb0_last_touched_diff <= 31769.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 6537.5f ) {
                                            return 137.0/168.1;
                                        } else {
                                            return 144.0/292.2;
                                        }
                                    } else {
                                        return 246.0/222.1;
                                    }
                                } else {
                                    return 153.0/104.1;
                                }
                            } else {
                                return 182.0/64.0;
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.934468686581f ) {
                                return 226.0/244.1;
                            } else {
                                if ( cl->stats.num_overlap_literals <= 115.5f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 13.0381946564f ) {
                                        if ( cl->stats.glue <= 10.5f ) {
                                            return 185.0/94.1;
                                        } else {
                                            return 202.0/68.0;
                                        }
                                    } else {
                                        return 173.0/114.1;
                                    }
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 3.5f ) {
                                        return 211.0/24.0;
                                    } else {
                                        return 227.0/60.0;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->size() <= 29.5f ) {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.size_rel <= 0.71236538887f ) {
                                    if ( cl->stats.sum_uip1_used <= 37.5f ) {
                                        return 98.0/334.2;
                                    } else {
                                        return 53.0/298.2;
                                    }
                                } else {
                                    return 100.0/240.1;
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 45.5f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 5464683.5f ) {
                                        return 143.0/282.2;
                                    } else {
                                        return 64.0/288.2;
                                    }
                                } else {
                                    return 140.0/202.1;
                                }
                            }
                        } else {
                            return 97.0/572.3;
                        }
                    } else {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.214041292667f ) {
                                return 112.0/288.2;
                            } else {
                                if ( cl->stats.num_overlap_literals <= 35.5f ) {
                                    return 96.0/202.1;
                                } else {
                                    return 235.0/234.1;
                                }
                            }
                        } else {
                            return 103.0/416.2;
                        }
                    }
                }
            }
        }
    } else {
        if ( cl->size() <= 11.5f ) {
            if ( rdb0_last_touched_diff <= 71156.0f ) {
                if ( cl->stats.glue_rel_long <= 0.646043419838f ) {
                    if ( rdb0_last_touched_diff <= 31383.0f ) {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 4428.0f ) {
                            return 156.0/324.2;
                        } else {
                            if ( cl->stats.num_overlap_literals <= 7.5f ) {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                    return 133.0/598.3;
                                } else {
                                    return 48.0/548.3;
                                }
                            } else {
                                if ( cl->size() <= 6.5f ) {
                                    return 68.0/338.2;
                                } else {
                                    return 125.0/346.2;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_long <= 0.574302554131f ) {
                            if ( cl->stats.num_antecedents_rel <= 0.0635159909725f ) {
                                return 165.0/226.1;
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.134712934494f ) {
                                    return 128.0/552.3;
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 12.5f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.173362255096f ) {
                                            return 139.0/242.1;
                                        } else {
                                            return 124.0/440.2;
                                        }
                                    } else {
                                        return 134.0/270.2;
                                    }
                                }
                            }
                        } else {
                            return 126.0/190.1;
                        }
                    }
                } else {
                    if ( cl->stats.num_overlap_literals <= 22.5f ) {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 610118.0f ) {
                            if ( cl->stats.size_rel <= 0.451735675335f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 20736.5f ) {
                                    return 106.0/232.1;
                                } else {
                                    return 157.0/210.1;
                                }
                            } else {
                                return 168.0/164.1;
                            }
                        } else {
                            return 78.0/274.2;
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 5.05723953247f ) {
                            return 265.0/248.1;
                        } else {
                            return 170.0/64.0;
                        }
                    }
                }
            } else {
                if ( cl->stats.antecedents_glue_long_reds_var <= 6.32010746002f ) {
                    if ( cl->stats.sum_uip1_used <= 43.5f ) {
                        if ( cl->stats.size_rel <= 0.399932324886f ) {
                            if ( cl->stats.dump_number <= 30.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0305796153843f ) {
                                    if ( cl->stats.size_rel <= 0.122568994761f ) {
                                        return 188.0/158.1;
                                    } else {
                                        return 177.0/94.1;
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 0.112326219678f ) {
                                        return 104.0/218.1;
                                    } else {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 49.5f ) {
                                            if ( cl->stats.num_antecedents_rel <= 0.285008370876f ) {
                                                return 224.0/136.1;
                                            } else {
                                                return 168.0/178.1;
                                            }
                                        } else {
                                            if ( cl->stats.glue <= 5.5f ) {
                                                if ( cl->size() <= 6.5f ) {
                                                    return 127.0/248.1;
                                                } else {
                                                    return 131.0/158.1;
                                                }
                                            } else {
                                                return 266.0/250.1;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.294779062271f ) {
                                    return 198.0/76.0;
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0633310824633f ) {
                                        return 272.0/152.1;
                                    } else {
                                        if ( cl->stats.sum_uip1_used <= 11.5f ) {
                                            return 196.0/112.1;
                                        } else {
                                            return 122.0/164.1;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 7.5f ) {
                                if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                    if ( cl->size() <= 9.5f ) {
                                        return 164.0/114.1;
                                    } else {
                                        return 315.0/128.1;
                                    }
                                } else {
                                    return 224.0/54.0;
                                }
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 730698.5f ) {
                                    return 121.0/160.1;
                                } else {
                                    return 198.0/140.1;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_act_ranking_top_10 <= 7.5f ) {
                            if ( cl->stats.size_rel <= 0.121205456555f ) {
                                return 106.0/206.1;
                            } else {
                                if ( cl->stats.sum_uip1_used <= 79.5f ) {
                                    return 140.0/318.2;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 116865.5f ) {
                                        return 65.0/404.2;
                                    } else {
                                        return 72.0/294.2;
                                    }
                                }
                            }
                        } else {
                            return 191.0/326.2;
                        }
                    }
                } else {
                    return 301.0/118.1;
                }
            }
        } else {
            if ( cl->stats.glue_rel_long <= 0.734749019146f ) {
                if ( cl->stats.sum_uip1_used <= 18.5f ) {
                    if ( rdb0_last_touched_diff <= 108109.5f ) {
                        if ( cl->stats.glue_rel_queue <= 0.491400539875f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.0450000017881f ) {
                                if ( cl->size() <= 24.0f ) {
                                    return 116.0/204.1;
                                } else {
                                    return 113.0/240.1;
                                }
                            } else {
                                if ( cl->stats.size_rel <= 0.492830455303f ) {
                                    return 162.0/196.1;
                                } else {
                                    return 186.0/84.0;
                                }
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 3.5f ) {
                                if ( cl->stats.glue <= 13.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 38.5f ) {
                                        return 157.0/110.1;
                                    } else {
                                        if ( rdb0_last_touched_diff <= 49316.5f ) {
                                            return 273.0/108.1;
                                        } else {
                                            return 270.0/56.0;
                                        }
                                    }
                                } else {
                                    return 181.0/156.1;
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                    return 239.0/182.1;
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 15.5f ) {
                                        return 129.0/162.1;
                                    } else {
                                        return 177.0/126.1;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 7.5f ) {
                            if ( cl->stats.size_rel <= 0.659994721413f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.245000004768f ) {
                                    return 175.0/36.0;
                                } else {
                                    return 225.0/18.0;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 34.5f ) {
                                    return 258.0/84.0;
                                } else {
                                    return 238.0/52.0;
                                }
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 5.5f ) {
                                if ( cl->stats.dump_number <= 23.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.276596009731f ) {
                                        return 242.0/88.0;
                                    } else {
                                        return 215.0/116.1;
                                    }
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 1616.5f ) {
                                        return 195.0/54.0;
                                    } else {
                                        return 310.0/44.0;
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 281365.0f ) {
                                    if ( cl->stats.size_rel <= 0.497073143721f ) {
                                        return 167.0/124.1;
                                    } else {
                                        return 289.0/142.1;
                                    }
                                } else {
                                    return 230.0/60.0;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                        if ( rdb0_last_touched_diff <= 153963.0f ) {
                            if ( cl->stats.sum_uip1_used <= 59.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 41404.0f ) {
                                    return 94.0/286.2;
                                } else {
                                    return 220.0/336.2;
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 50112.0f ) {
                                    return 36.0/346.2;
                                } else {
                                    return 117.0/396.2;
                                }
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals <= 21.5f ) {
                                return 209.0/270.2;
                            } else {
                                return 161.0/136.1;
                            }
                        }
                    } else {
                        return 61.0/480.3;
                    }
                }
            } else {
                if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                    if ( cl->stats.sum_delta_confl_uip1_used <= 1958.0f ) {
                        if ( cl->stats.num_overlap_literals_rel <= 0.133644521236f ) {
                            if ( cl->stats.num_overlap_literals_rel <= 0.11470630765f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.569444417953f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 55779.5f ) {
                                        return 220.0/120.1;
                                    } else {
                                        if ( rdb0_last_touched_diff <= 156966.0f ) {
                                            return 195.0/28.0;
                                        } else {
                                            return 195.0/88.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.993098199368f ) {
                                        return 307.0/80.0;
                                    } else {
                                        if ( cl->size() <= 57.5f ) {
                                            return 234.0/38.0;
                                        } else {
                                            return 194.0/8.0;
                                        }
                                    }
                                }
                            } else {
                                return 193.0/114.1;
                            }
                        } else {
                            if ( cl->stats.glue <= 11.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 39.5f ) {
                                    return 209.0/90.1;
                                } else {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 8.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.404939711094f ) {
                                            if ( rdb0_last_touched_diff <= 102364.5f ) {
                                                return 178.0/22.0;
                                            } else {
                                                return 193.0/16.0;
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 1.17614018917f ) {
                                                if ( cl->stats.dump_number <= 4.5f ) {
                                                    return 279.0/116.1;
                                                } else {
                                                    if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                                        if ( cl->stats.dump_number <= 15.5f ) {
                                                            if ( cl->stats.antecedents_glue_long_reds_var <= 3.61269235611f ) {
                                                                return 203.0/30.0;
                                                            } else {
                                                                return 214.0/46.0;
                                                            }
                                                        } else {
                                                            return 231.0/12.0;
                                                        }
                                                    } else {
                                                        return 345.0/104.1;
                                                    }
                                                }
                                            } else {
                                                return 229.0/92.1;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.size_rel <= 0.924244165421f ) {
                                            return 182.0/34.0;
                                        } else {
                                            return 271.0/12.0;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.size_rel <= 0.244340375066f ) {
                                    return 205.0/76.0;
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 1.07098662853f ) {
                                            if ( rdb0_last_touched_diff <= 108304.0f ) {
                                                if ( cl->stats.glue <= 14.5f ) {
                                                    return 292.0/24.0;
                                                } else {
                                                    if ( cl->stats.size_rel <= 1.10220897198f ) {
                                                        return 290.0/42.0;
                                                    } else {
                                                        return 203.0/64.0;
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.size_rel <= 0.754149913788f ) {
                                                    return 189.0/22.0;
                                                } else {
                                                    if ( cl->stats.num_total_lits_antecedents <= 165.5f ) {
                                                        return 211.0/6.0;
                                                    } else {
                                                        return 252.0/2.0;
                                                    }
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.num_overlap_literals <= 305.5f ) {
                                                if ( rdb0_last_touched_diff <= 27813.0f ) {
                                                    return 212.0/60.0;
                                                } else {
                                                    if ( cl->stats.num_antecedents_rel <= 1.24203658104f ) {
                                                        if ( cl->stats.dump_number <= 5.5f ) {
                                                            return 303.0/16.0;
                                                        } else {
                                                            if ( cl->size() <= 37.5f ) {
                                                                if ( cl->stats.num_antecedents_rel <= 0.543087601662f ) {
                                                                    return 187.0/12.0;
                                                                } else {
                                                                    return 297.0/4.0;
                                                                }
                                                            } else {
                                                                return 393.0/2.0;
                                                            }
                                                        }
                                                    } else {
                                                        return 370.0/38.0;
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.size_rel <= 0.755694508553f ) {
                                                    return 235.0/6.0;
                                                } else {
                                                    if ( rdb0_last_touched_diff <= 63207.5f ) {
                                                        return 1;
                                                    } else {
                                                        return 356.0/4.0;
                                                    }
                                                }
                                            }
                                        }
                                    } else {
                                        if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                            if ( cl->stats.num_overlap_literals <= 117.5f ) {
                                                if ( cl->stats.num_antecedents_rel <= 0.376159965992f ) {
                                                    if ( cl->stats.glue_rel_queue <= 1.11352670193f ) {
                                                        return 246.0/16.0;
                                                    } else {
                                                        return 225.0/52.0;
                                                    }
                                                } else {
                                                    if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                                        return 290.0/58.0;
                                                    } else {
                                                        if ( rdb0_last_touched_diff <= 277574.0f ) {
                                                            return 347.0/90.1;
                                                        } else {
                                                            return 182.0/76.0;
                                                        }
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.num_antecedents_rel <= 3.16699934006f ) {
                                                    if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                                        if ( cl->size() <= 87.5f ) {
                                                            return 300.0/30.0;
                                                        } else {
                                                            return 208.0/2.0;
                                                        }
                                                    } else {
                                                        if ( cl->stats.glue_rel_long <= 1.25390923023f ) {
                                                            if ( cl->stats.sum_delta_confl_uip1_used <= 3.5f ) {
                                                                return 255.0/58.0;
                                                            } else {
                                                                return 195.0/20.0;
                                                            }
                                                        } else {
                                                            if ( cl->stats.num_antecedents_rel <= 1.13694393635f ) {
                                                                return 236.0/26.0;
                                                            } else {
                                                                return 241.0/12.0;
                                                            }
                                                        }
                                                    }
                                                } else {
                                                    return 184.0/54.0;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 1.08793234825f ) {
                                                if ( cl->size() <= 63.5f ) {
                                                    return 360.0/40.0;
                                                } else {
                                                    return 192.0/42.0;
                                                }
                                            } else {
                                                if ( rdb0_last_touched_diff <= 99519.5f ) {
                                                    return 197.0/36.0;
                                                } else {
                                                    if ( cl->stats.sum_delta_confl_uip1_used <= 10.5f ) {
                                                        if ( cl->stats.dump_number <= 24.5f ) {
                                                            if ( cl->stats.dump_number <= 15.5f ) {
                                                                return 266.0/12.0;
                                                            } else {
                                                                return 283.0/2.0;
                                                            }
                                                        } else {
                                                            return 288.0/18.0;
                                                        }
                                                    } else {
                                                        return 190.0/18.0;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.size_rel <= 0.631453752518f ) {
                            if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                return 155.0/224.1;
                            } else {
                                if ( cl->stats.sum_uip1_used <= 29.0f ) {
                                    if ( rdb0_last_touched_diff <= 127557.0f ) {
                                        if ( cl->stats.glue <= 12.5f ) {
                                            return 206.0/112.1;
                                        } else {
                                            return 147.0/150.1;
                                        }
                                    } else {
                                        return 361.0/122.1;
                                    }
                                } else {
                                    return 73.0/250.1;
                                }
                            }
                        } else {
                            if ( cl->stats.size_rel <= 2.03137898445f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 793941.0f ) {
                                    if ( rdb0_last_touched_diff <= 43765.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 18689.5f ) {
                                            return 173.0/122.1;
                                        } else {
                                            return 152.0/152.1;
                                        }
                                    } else {
                                        if ( cl->stats.sum_uip1_used <= 4.5f ) {
                                            if ( cl->stats.num_overlap_literals <= 29.5f ) {
                                                return 264.0/70.0;
                                            } else {
                                                if ( cl->stats.antecedents_glue_long_reds_var <= 14.2916660309f ) {
                                                    return 343.0/40.0;
                                                } else {
                                                    return 186.0/46.0;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.glue <= 11.5f ) {
                                                return 225.0/140.1;
                                            } else {
                                                if ( cl->stats.glue_rel_queue <= 1.04895138741f ) {
                                                    return 353.0/72.0;
                                                } else {
                                                    return 208.0/106.1;
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 5636863.5f ) {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 1791078.0f ) {
                                            return 147.0/160.1;
                                        } else {
                                            return 154.0/102.1;
                                        }
                                    } else {
                                        return 100.0/222.1;
                                    }
                                }
                            } else {
                                return 227.0/44.0;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.dump_number <= 1.5f ) {
                        return 162.0/172.1;
                    } else {
                        return 90.0/342.2;
                    }
                }
            }
        }
    }
}

static double estimator_should_keep_long_conf2_cluster0_2(
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
        if ( cl->stats.glue_rel_queue <= 0.778240323067f ) {
            if ( cl->stats.sum_delta_confl_uip1_used <= 579202.0f ) {
                if ( cl->stats.rdb1_last_touched_diff <= 97030.5f ) {
                    if ( cl->size() <= 10.5f ) {
                        if ( cl->stats.sum_uip1_used <= 11.5f ) {
                            if ( rdb0_last_touched_diff <= 26582.0f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 2570.0f ) {
                                    return 133.0/512.3;
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0545228421688f ) {
                                            return 70.0/240.1;
                                        } else {
                                            return 67.0/300.2;
                                        }
                                    } else {
                                        if ( cl->size() <= 7.5f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0516428425908f ) {
                                                return 115.0/186.1;
                                            } else {
                                                return 121.0/312.2;
                                            }
                                        } else {
                                            return 185.0/274.2;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0508920401335f ) {
                                        return 111.0/248.1;
                                    } else {
                                        return 111.0/316.2;
                                    }
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.510354995728f ) {
                                        if ( cl->stats.glue_rel_long <= 0.618624091148f ) {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 126597.5f ) {
                                                if ( cl->stats.glue_rel_queue <= 0.336511611938f ) {
                                                    return 152.0/356.2;
                                                } else {
                                                    if ( cl->stats.glue_rel_queue <= 0.453928291798f ) {
                                                        return 160.0/172.1;
                                                    } else {
                                                        return 195.0/418.2;
                                                    }
                                                }
                                            } else {
                                                return 173.0/114.1;
                                            }
                                        } else {
                                            return 165.0/170.1;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.574409604073f ) {
                                            return 130.0/176.1;
                                        } else {
                                            return 139.0/110.1;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.dump_number <= 9.5f ) {
                                if ( cl->stats.glue_rel_queue <= 0.575407862663f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 22.5f ) {
                                        if ( cl->stats.glue_rel_long <= 0.413895905018f ) {
                                            return 116.0/676.4;
                                        } else {
                                            return 25.0/332.2;
                                        }
                                    } else {
                                        return 70.0/326.2;
                                    }
                                } else {
                                    return 74.0/286.2;
                                }
                            } else {
                                return 131.0/316.2;
                            }
                        }
                    } else {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 137.0f ) {
                            if ( cl->stats.num_overlap_literals_rel <= 0.181145563722f ) {
                                if ( cl->stats.size_rel <= 0.557179808617f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 46.5f ) {
                                        return 199.0/280.2;
                                    } else {
                                        return 200.0/118.1;
                                    }
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.219482570887f ) {
                                        return 157.0/94.1;
                                    } else {
                                        return 187.0/90.1;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.734786450863f ) {
                                    if ( cl->stats.num_overlap_literals <= 93.5f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 76.5f ) {
                                            return 190.0/82.0;
                                        } else {
                                            return 146.0/112.1;
                                        }
                                    } else {
                                        return 239.0/56.0;
                                    }
                                } else {
                                    return 186.0/44.0;
                                }
                            }
                        } else {
                            if ( cl->stats.size_rel <= 0.716346740723f ) {
                                if ( cl->stats.dump_number <= 12.5f ) {
                                    if ( rdb0_last_touched_diff <= 29967.5f ) {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 269160.5f ) {
                                            if ( cl->stats.glue_rel_long <= 0.662340283394f ) {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.235107004642f ) {
                                                    if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                                                        return 190.0/372.2;
                                                    } else {
                                                        return 70.0/264.1;
                                                    }
                                                } else {
                                                    return 119.0/208.1;
                                                }
                                            } else {
                                                return 137.0/170.1;
                                            }
                                        } else {
                                            return 56.0/274.2;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.659628629684f ) {
                                            if ( cl->stats.sum_uip1_used <= 17.5f ) {
                                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.0546875f ) {
                                                    return 173.0/344.2;
                                                } else {
                                                    if ( cl->stats.num_antecedents_rel <= 0.182461857796f ) {
                                                        return 168.0/146.1;
                                                    } else {
                                                        return 149.0/176.1;
                                                    }
                                                }
                                            } else {
                                                return 76.0/296.2;
                                            }
                                        } else {
                                            return 193.0/214.1;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 43.5f ) {
                                        return 179.0/86.0;
                                    } else {
                                        return 137.0/138.1;
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 46816.0f ) {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                        return 232.0/278.2;
                                    } else {
                                        return 213.0/156.1;
                                    }
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                        return 224.0/42.0;
                                    } else {
                                        return 171.0/82.0;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.glue_rel_queue <= 0.488864898682f ) {
                        if ( cl->stats.dump_number <= 23.5f ) {
                            if ( cl->stats.size_rel <= 0.466021120548f ) {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 6.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 21.5f ) {
                                        return 126.0/178.1;
                                    } else {
                                        return 107.0/190.1;
                                    }
                                } else {
                                    return 219.0/206.1;
                                }
                            } else {
                                return 201.0/72.0;
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 52.5f ) {
                                if ( cl->size() <= 7.5f ) {
                                    return 164.0/178.1;
                                } else {
                                    return 312.0/168.1;
                                }
                            } else {
                                return 192.0/34.0;
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 105302.0f ) {
                            return 169.0/134.1;
                        } else {
                            if ( cl->stats.glue <= 7.5f ) {
                                if ( cl->size() <= 9.5f ) {
                                    if ( cl->stats.glue <= 5.5f ) {
                                        return 226.0/110.1;
                                    } else {
                                        return 191.0/160.1;
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.362340152264f ) {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 226.5f ) {
                                            return 192.0/42.0;
                                        } else {
                                            return 226.0/74.0;
                                        }
                                    } else {
                                        return 273.0/124.1;
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 2190.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.590582966805f ) {
                                        return 194.0/84.0;
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 47.5f ) {
                                            return 231.0/74.0;
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 161180.0f ) {
                                                return 180.0/36.0;
                                            } else {
                                                return 348.0/40.0;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->size() <= 29.5f ) {
                                        if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                            return 182.0/138.1;
                                        } else {
                                            return 230.0/80.0;
                                        }
                                    } else {
                                        return 226.0/68.0;
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.rdb1_last_touched_diff <= 45264.5f ) {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 51.5f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 13.5f ) {
                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0368069186807f ) {
                                        if ( cl->stats.size_rel <= 0.140097796917f ) {
                                            return 53.0/326.2;
                                        } else {
                                            return 57.0/320.2;
                                        }
                                    } else {
                                        return 38.0/384.2;
                                    }
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 95.5f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.151512652636f ) {
                                            return 159.0/410.2;
                                        } else {
                                            return 124.0/212.1;
                                        }
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0642282068729f ) {
                                            return 48.0/342.2;
                                        } else {
                                            return 22.0/408.2;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.dump_number <= 15.5f ) {
                                    if ( cl->stats.sum_uip1_used <= 45.5f ) {
                                        return 127.0/582.3;
                                    } else {
                                        if ( cl->stats.glue <= 5.5f ) {
                                            return 21.0/408.2;
                                        } else {
                                            return 48.0/508.3;
                                        }
                                    }
                                } else {
                                    if ( cl->size() <= 10.5f ) {
                                        if ( cl->stats.sum_uip1_used <= 82.5f ) {
                                            if ( rdb0_last_touched_diff <= 30663.5f ) {
                                                if ( rdb0_last_touched_diff <= 20978.0f ) {
                                                    return 158.0/236.1;
                                                } else {
                                                    return 83.0/258.1;
                                                }
                                            } else {
                                                return 202.0/278.2;
                                            }
                                        } else {
                                            if ( cl->stats.dump_number <= 54.5f ) {
                                                if ( cl->stats.num_overlap_literals <= 8.5f ) {
                                                    return 58.0/430.2;
                                                } else {
                                                    return 36.0/454.3;
                                                }
                                            } else {
                                                return 90.0/398.2;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.149444445968f ) {
                                            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                                return 84.0/280.2;
                                            } else {
                                                return 104.0/244.1;
                                            }
                                        } else {
                                            if ( cl->stats.dump_number <= 39.5f ) {
                                                return 141.0/320.2;
                                            } else {
                                                return 128.0/162.1;
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.520586967468f ) {
                                if ( cl->stats.dump_number <= 20.5f ) {
                                    return 73.0/396.2;
                                } else {
                                    return 102.0/224.1;
                                }
                            } else {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                    return 164.0/294.2;
                                } else {
                                    return 136.0/218.1;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 22.5f ) {
                            if ( cl->stats.sum_uip1_used <= 149.5f ) {
                                if ( cl->stats.dump_number <= 13.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.128878295422f ) {
                                        return 55.0/314.2;
                                    } else {
                                        return 31.0/434.2;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0543185472488f ) {
                                        return 94.0/266.2;
                                    } else {
                                        return 86.0/480.3;
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 14410.0f ) {
                                    return 24.0/598.3;
                                } else {
                                    return 25.0/366.2;
                                }
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 207.0f ) {
                                return 33.0/328.2;
                            } else {
                                return 8.0/428.2;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.sum_delta_confl_uip1_used <= 8363419.0f ) {
                        if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                            if ( cl->stats.num_overlap_literals <= 21.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 20.5f ) {
                                    if ( cl->stats.sum_uip1_used <= 36.5f ) {
                                        if ( cl->stats.size_rel <= 0.222726792097f ) {
                                            return 136.0/182.1;
                                        } else {
                                            return 132.0/138.1;
                                        }
                                    } else {
                                        return 114.0/440.2;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 9.5f ) {
                                        return 197.0/174.1;
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 31.5f ) {
                                            return 160.0/212.1;
                                        } else {
                                            return 135.0/248.1;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 1699064.5f ) {
                                    return 156.0/90.1;
                                } else {
                                    return 157.0/148.1;
                                }
                            }
                        } else {
                            if ( cl->stats.dump_number <= 28.5f ) {
                                return 104.0/318.2;
                            } else {
                                if ( cl->stats.sum_uip1_used <= 21.5f ) {
                                    return 343.0/122.1;
                                } else {
                                    return 261.0/272.2;
                                }
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 166648.5f ) {
                            if ( cl->stats.num_overlap_literals_rel <= 0.0243922881782f ) {
                                return 89.0/228.1;
                            } else {
                                if ( cl->stats.dump_number <= 43.5f ) {
                                    return 61.0/456.3;
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.496169954538f ) {
                                        return 132.0/442.2;
                                    } else {
                                        return 106.0/266.2;
                                    }
                                }
                            }
                        } else {
                            if ( cl->size() <= 12.5f ) {
                                return 198.0/408.2;
                            } else {
                                return 146.0/174.1;
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.num_overlap_literals <= 27.5f ) {
                if ( cl->stats.sum_delta_confl_uip1_used <= 2235.0f ) {
                    if ( cl->stats.dump_number <= 5.5f ) {
                        if ( cl->stats.glue_rel_queue <= 1.05553555489f ) {
                            if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                                return 253.0/322.2;
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 1.5f ) {
                                    return 166.0/98.1;
                                } else {
                                    return 229.0/184.1;
                                }
                            }
                        } else {
                            if ( cl->stats.num_antecedents_rel <= 0.147687554359f ) {
                                return 211.0/52.0;
                            } else {
                                return 279.0/204.1;
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 97679.0f ) {
                            if ( cl->stats.num_overlap_literals <= 10.5f ) {
                                return 221.0/128.1;
                            } else {
                                if ( cl->stats.size_rel <= 0.689353168011f ) {
                                    return 156.0/88.0;
                                } else {
                                    return 300.0/70.0;
                                }
                            }
                        } else {
                            if ( cl->stats.glue <= 7.5f ) {
                                return 172.0/90.1;
                            } else {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.527777791023f ) {
                                    if ( cl->stats.glue_rel_queue <= 1.03520393372f ) {
                                        return 275.0/118.1;
                                    } else {
                                        return 251.0/32.0;
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 322646.0f ) {
                                        if ( cl->stats.dump_number <= 17.5f ) {
                                            return 305.0/40.0;
                                        } else {
                                            return 302.0/18.0;
                                        }
                                    } else {
                                        return 227.0/48.0;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_last_touched_diff <= 39771.0f ) {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 1086046.0f ) {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.288831472397f ) {
                                if ( cl->stats.size_rel <= 0.298953711987f ) {
                                    return 99.0/318.2;
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.911926686764f ) {
                                        return 157.0/354.2;
                                    } else {
                                        return 171.0/238.1;
                                    }
                                }
                            } else {
                                return 179.0/250.1;
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 26.5f ) {
                                return 84.0/446.3;
                            } else {
                                if ( cl->stats.glue <= 11.5f ) {
                                    return 119.0/354.2;
                                } else {
                                    return 95.0/352.2;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 2618253.5f ) {
                            if ( rdb0_last_touched_diff <= 302220.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.0694444477558f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.946909666061f ) {
                                        return 169.0/194.1;
                                    } else {
                                        return 201.0/158.1;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.845559835434f ) {
                                        return 160.0/136.1;
                                    } else {
                                        if ( cl->stats.sum_uip1_used <= 5.5f ) {
                                            return 207.0/48.0;
                                        } else {
                                            return 195.0/118.1;
                                        }
                                    }
                                }
                            } else {
                                return 242.0/48.0;
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 114981.0f ) {
                                return 120.0/298.2;
                            } else {
                                return 152.0/174.1;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.glue <= 10.5f ) {
                    if ( rdb0_last_touched_diff <= 78952.5f ) {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 274.0f ) {
                            if ( cl->size() <= 13.5f ) {
                                if ( cl->size() <= 9.5f ) {
                                    return 209.0/166.1;
                                } else {
                                    return 203.0/116.1;
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 95.5f ) {
                                    return 233.0/120.1;
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.707849502563f ) {
                                        return 205.0/26.0;
                                    } else {
                                        return 246.0/50.0;
                                    }
                                }
                            }
                        } else {
                            if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                if ( cl->stats.sum_uip1_used <= 17.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.892374515533f ) {
                                        return 170.0/200.1;
                                    } else {
                                        return 255.0/212.1;
                                    }
                                } else {
                                    return 112.0/346.2;
                                }
                            } else {
                                return 185.0/146.1;
                            }
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 2.16246533394f ) {
                            if ( cl->stats.dump_number <= 25.5f ) {
                                return 368.0/96.1;
                            } else {
                                return 171.0/136.1;
                            }
                        } else {
                            if ( cl->size() <= 20.5f ) {
                                if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                    return 328.0/58.0;
                                } else {
                                    return 301.0/118.1;
                                }
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 37.5f ) {
                                    return 309.0/20.0;
                                } else {
                                    return 194.0/34.0;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_total_lits_antecedents <= 510.5f ) {
                        if ( cl->stats.sum_uip1_used <= 11.5f ) {
                            if ( cl->stats.sum_uip1_used <= 2.5f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 85.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 133.5f ) {
                                        if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.306912899017f ) {
                                                return 243.0/8.0;
                                            } else {
                                                if ( cl->stats.size_rel <= 0.876246631145f ) {
                                                    return 220.0/52.0;
                                                } else {
                                                    if ( cl->stats.glue_rel_long <= 1.22532808781f ) {
                                                        return 403.0/8.0;
                                                    } else {
                                                        return 223.0/16.0;
                                                    }
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.antec_num_total_lits_rel <= 1.05159974098f ) {
                                                if ( cl->stats.num_antecedents_rel <= 0.474105536938f ) {
                                                    if ( cl->stats.num_overlap_literals <= 50.5f ) {
                                                        return 348.0/68.0;
                                                    } else {
                                                        return 232.0/14.0;
                                                    }
                                                } else {
                                                    if ( rdb0_last_touched_diff <= 128986.5f ) {
                                                        return 208.0/104.1;
                                                    } else {
                                                        if ( cl->size() <= 24.5f ) {
                                                            return 190.0/72.0;
                                                        } else {
                                                            return 206.0/40.0;
                                                        }
                                                    }
                                                }
                                            } else {
                                                return 192.0/14.0;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.dump_number <= 45.5f ) {
                                            if ( cl->stats.num_antecedents_rel <= 1.44346475601f ) {
                                                if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                                    if ( cl->stats.sum_delta_confl_uip1_used <= 7.5f ) {
                                                        if ( cl->stats.dump_number <= 2.5f ) {
                                                            if ( cl->stats.num_overlap_literals <= 135.5f ) {
                                                                return 198.0/16.0;
                                                            } else {
                                                                return 188.0/34.0;
                                                            }
                                                        } else {
                                                            if ( cl->stats.num_overlap_literals_rel <= 0.356850206852f ) {
                                                                return 195.0/10.0;
                                                            } else {
                                                                if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                                                                    return 306.0/10.0;
                                                                } else {
                                                                    if ( cl->stats.dump_number <= 9.5f ) {
                                                                        return 300.0/4.0;
                                                                    } else {
                                                                        return 1;
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    } else {
                                                        return 351.0/40.0;
                                                    }
                                                } else {
                                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 8.5f ) {
                                                        if ( cl->stats.rdb1_last_touched_diff <= 163090.5f ) {
                                                            if ( cl->stats.antec_num_total_lits_rel <= 0.630121350288f ) {
                                                                return 234.0/48.0;
                                                            } else {
                                                                if ( cl->stats.num_overlap_literals_rel <= 1.01615297794f ) {
                                                                    return 221.0/6.0;
                                                                } else {
                                                                    return 252.0/36.0;
                                                                }
                                                            }
                                                        } else {
                                                            return 298.0/72.0;
                                                        }
                                                    } else {
                                                        if ( cl->stats.num_antecedents_rel <= 0.706881523132f ) {
                                                            return 283.0/10.0;
                                                        } else {
                                                            return 219.0/24.0;
                                                        }
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.num_antecedents_rel <= 1.57310688496f ) {
                                                    return 182.0/44.0;
                                                } else {
                                                    if ( cl->stats.antecedents_glue_long_reds_var <= 12.1769256592f ) {
                                                        return 378.0/30.0;
                                                    } else {
                                                        if ( rdb0_last_touched_diff <= 63437.5f ) {
                                                            return 193.0/50.0;
                                                        } else {
                                                            return 297.0/30.0;
                                                        }
                                                    }
                                                }
                                            }
                                        } else {
                                            return 231.0/62.0;
                                        }
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 72318.5f ) {
                                        return 223.0/108.1;
                                    } else {
                                        return 357.0/76.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 30006.0f ) {
                                    if ( cl->stats.size_rel <= 0.793750524521f ) {
                                        return 164.0/126.1;
                                    } else {
                                        return 290.0/98.1;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 1.30781722069f ) {
                                        if ( cl->stats.glue_rel_long <= 1.01640319824f ) {
                                            if ( cl->stats.glue <= 16.5f ) {
                                                return 334.0/108.1;
                                            } else {
                                                return 214.0/36.0;
                                            }
                                        } else {
                                            if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                                return 306.0/14.0;
                                            } else {
                                                return 269.0/60.0;
                                            }
                                        }
                                    } else {
                                        return 188.0/82.0;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 111431.5f ) {
                                if ( cl->size() <= 54.5f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 1452106.0f ) {
                                        return 131.0/214.1;
                                    } else {
                                        return 73.0/276.2;
                                    }
                                } else {
                                    return 167.0/266.2;
                                }
                            } else {
                                return 171.0/90.1;
                            }
                        }
                    } else {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 56082.5f ) {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 511.5f ) {
                                if ( cl->stats.glue_rel_long <= 0.914879918098f ) {
                                    return 233.0/44.0;
                                } else {
                                    if ( cl->stats.glue <= 14.5f ) {
                                        return 244.0/28.0;
                                    } else {
                                        if ( rdb0_last_touched_diff <= 22760.0f ) {
                                            return 275.0/30.0;
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 1.10439741611f ) {
                                                if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                                    return 318.0/8.0;
                                                } else {
                                                    if ( cl->stats.size_rel <= 1.05100274086f ) {
                                                        return 193.0/20.0;
                                                    } else {
                                                        return 241.0/16.0;
                                                    }
                                                }
                                            } else {
                                                if ( cl->size() <= 95.5f ) {
                                                    if ( rdb0_last_touched_diff <= 66825.0f ) {
                                                        return 211.0/10.0;
                                                    } else {
                                                        return 354.0/6.0;
                                                    }
                                                } else {
                                                    if ( cl->stats.glue_rel_long <= 1.61341869831f ) {
                                                        return 1;
                                                    } else {
                                                        return 335.0/6.0;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            } else {
                                return 259.0/50.0;
                            }
                        } else {
                            return 206.0/98.1;
                        }
                    }
                }
            }
        }
    } else {
        if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
            if ( cl->stats.sum_delta_confl_uip1_used <= 4647555.5f ) {
                if ( cl->size() <= 9.5f ) {
                    if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                            if ( cl->stats.size_rel <= 0.139455884695f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 274415.0f ) {
                                    return 37.0/378.2;
                                } else {
                                    return 85.0/392.2;
                                }
                            } else {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                    if ( cl->stats.num_overlap_literals <= 5.5f ) {
                                        return 97.0/426.2;
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.52546620369f ) {
                                            return 53.0/294.2;
                                        } else {
                                            return 45.0/348.2;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0568041056395f ) {
                                        return 110.0/214.1;
                                    } else {
                                        return 118.0/388.2;
                                    }
                                }
                            }
                        } else {
                            return 164.0/418.2;
                        }
                    } else {
                        if ( cl->stats.sum_uip1_used <= 30.5f ) {
                            if ( cl->size() <= 7.5f ) {
                                if ( rdb0_last_touched_diff <= 2208.0f ) {
                                    return 83.0/612.3;
                                } else {
                                    return 54.0/674.4;
                                }
                            } else {
                                return 76.0/380.2;
                            }
                        } else {
                            if ( cl->stats.num_antecedents_rel <= 0.0941878706217f ) {
                                return 48.0/496.3;
                            } else {
                                if ( cl->stats.dump_number <= 8.5f ) {
                                    return 9.0/586.3;
                                } else {
                                    return 24.0/406.2;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.166419744492f ) {
                            if ( cl->stats.size_rel <= 0.607881188393f ) {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                    return 85.0/298.2;
                                } else {
                                    return 154.0/320.2;
                                }
                            } else {
                                return 81.0/336.2;
                            }
                        } else {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.182225972414f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.11552157253f ) {
                                    return 120.0/170.1;
                                } else {
                                    return 71.0/272.2;
                                }
                            } else {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 10.9131946564f ) {
                                    if ( cl->stats.size_rel <= 0.651955604553f ) {
                                        return 224.0/342.2;
                                    } else {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 173587.0f ) {
                                            return 158.0/106.1;
                                        } else {
                                            return 159.0/168.1;
                                        }
                                    }
                                } else {
                                    return 162.0/88.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.sum_uip1_used <= 32.5f ) {
                            if ( cl->stats.glue_rel_queue <= 0.574717998505f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.119271084666f ) {
                                    return 103.0/234.1;
                                } else {
                                    return 68.0/526.3;
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 72.5f ) {
                                    if ( cl->stats.glue <= 8.5f ) {
                                        return 105.0/306.2;
                                    } else {
                                        return 72.0/432.2;
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 0.75645160675f ) {
                                        return 124.0/292.2;
                                    } else {
                                        return 116.0/156.1;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 2894689.0f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.0694444477558f ) {
                                    return 30.0/454.3;
                                } else {
                                    return 49.0/416.2;
                                }
                            } else {
                                return 61.0/318.2;
                            }
                        }
                    }
                }
            } else {
                if ( rdb0_last_touched_diff <= 1769.5f ) {
                    if ( cl->stats.num_overlap_literals <= 16.5f ) {
                        if ( cl->stats.dump_number <= 29.5f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 17.5f ) {
                                return 16.0/724.4;
                            } else {
                                return 33.0/650.4;
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.420602560043f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 84670256.0f ) {
                                    return 38.0/416.2;
                                } else {
                                    return 12.0/434.2;
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 18.5f ) {
                                    return 40.0/332.2;
                                } else {
                                    return 32.0/404.2;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 12823898.0f ) {
                            return 48.0/320.2;
                        } else {
                            return 27.0/558.3;
                        }
                    }
                } else {
                    if ( cl->stats.size_rel <= 0.884437561035f ) {
                        if ( cl->size() <= 8.5f ) {
                            if ( cl->stats.sum_uip1_used <= 99.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 13539.5f ) {
                                    return 57.0/462.3;
                                } else {
                                    return 68.0/312.2;
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.258855462074f ) {
                                    return 48.0/402.2;
                                } else {
                                    if ( rdb0_last_touched_diff <= 2980.0f ) {
                                        return 32.0/384.2;
                                    } else {
                                        if ( cl->stats.size_rel <= 0.274789005518f ) {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 39212864.0f ) {
                                                return 19.0/444.3;
                                            } else {
                                                return 16.0/500.3;
                                            }
                                        } else {
                                            return 27.0/458.3;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 115.5f ) {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                    return 83.0/428.2;
                                } else {
                                    return 91.0/232.1;
                                }
                            } else {
                                if ( cl->stats.dump_number <= 30.5f ) {
                                    return 30.0/476.3;
                                } else {
                                    return 62.0/424.2;
                                }
                            }
                        }
                    } else {
                        return 78.0/326.2;
                    }
                }
            }
        } else {
            if ( cl->stats.glue_rel_long <= 0.604743778706f ) {
                if ( cl->stats.sum_uip1_used <= 56.5f ) {
                    if ( cl->stats.num_total_lits_antecedents <= 32.5f ) {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 1144778.5f ) {
                            if ( cl->stats.sum_uip1_used <= 14.5f ) {
                                return 62.0/412.2;
                            } else {
                                if ( cl->size() <= 4.5f ) {
                                    return 21.0/638.4;
                                } else {
                                    if ( rdb0_last_touched_diff <= 1433.5f ) {
                                        return 15.0/548.3;
                                    } else {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 343205.0f ) {
                                            return 28.0/468.3;
                                        } else {
                                            return 44.0/410.2;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 2427.0f ) {
                                return 32.0/476.3;
                            } else {
                                return 49.0/314.2;
                            }
                        }
                    } else {
                        if ( cl->stats.sum_uip1_used <= 22.5f ) {
                            return 73.0/330.2;
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 1299.0f ) {
                                return 61.0/356.2;
                            } else {
                                return 28.0/458.3;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.antec_num_total_lits_rel <= 0.00398506131023f ) {
                        return 33.0/668.4;
                    } else {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 23.5f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 31.5f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 35844940.0f ) {
                                    if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.0829974412918f ) {
                                            return 8.0/476.3;
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 0.36141872406f ) {
                                                return 29.0/358.2;
                                            } else {
                                                return 23.0/424.2;
                                            }
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 3815.5f ) {
                                            if ( cl->stats.glue_rel_queue <= 0.20697209239f ) {
                                                return 14.0/374.2;
                                            } else {
                                                if ( cl->stats.num_overlap_literals <= 4.5f ) {
                                                    if ( cl->stats.sum_delta_confl_uip1_used <= 12001316.0f ) {
                                                        return 21.0/664.4;
                                                    } else {
                                                        return 3.0/396.2;
                                                    }
                                                } else {
                                                    if ( cl->stats.used_for_uip_creation <= 24.5f ) {
                                                        if ( cl->stats.dump_number <= 13.5f ) {
                                                            return 0.0/544.3;
                                                        } else {
                                                            return 5.0/446.3;
                                                        }
                                                    } else {
                                                        return 11.0/468.3;
                                                    }
                                                }
                                            }
                                        } else {
                                            return 35.0/508.3;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 38.5f ) {
                                        if ( cl->stats.used_for_uip_creation <= 11.5f ) {
                                            return 8.0/434.2;
                                        } else {
                                            return 2.0/624.4;
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0292464196682f ) {
                                            return 28.0/454.3;
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 0.421134889126f ) {
                                                return 2.0/740.4;
                                            } else {
                                                return 4.0/458.3;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                    return 38.0/380.2;
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.329595953226f ) {
                                        return 46.0/668.4;
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0582380332053f ) {
                                            return 16.0/374.2;
                                        } else {
                                            if ( rdb0_last_touched_diff <= 1408.0f ) {
                                                return 4.0/478.3;
                                            } else {
                                                return 7.0/366.2;
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.0269206576049f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 401.0f ) {
                                    return 10.0/728.4;
                                } else {
                                    return 28.0/738.4;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 7.5f ) {
                                    if ( cl->stats.used_for_uip_creation <= 40.5f ) {
                                        if ( rdb0_last_touched_diff <= 3884.5f ) {
                                            if ( cl->stats.dump_number <= 19.5f ) {
                                                return 0.0/828.5;
                                            } else {
                                                return 2.0/578.3;
                                            }
                                        } else {
                                            return 7.0/480.3;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 42.5f ) {
                                            return 8.0/384.2;
                                        } else {
                                            if ( cl->size() <= 5.5f ) {
                                                return 0.0/490.3;
                                            } else {
                                                return 11.0/488.3;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 0.258718252182f ) {
                                        if ( cl->stats.sum_uip1_used <= 573.0f ) {
                                            if ( cl->stats.num_overlap_literals <= 26.5f ) {
                                                return 12.0/800.5;
                                            } else {
                                                return 12.0/420.2;
                                            }
                                        } else {
                                            return 28.0/616.3;
                                        }
                                    } else {
                                        if ( cl->stats.size_rel <= 0.376101911068f ) {
                                            return 4.0/664.4;
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 403.0f ) {
                                                return 15.0/386.2;
                                            } else {
                                                return 2.0/414.2;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.used_for_uip_creation <= 7.5f ) {
                    if ( cl->stats.glue <= 19.5f ) {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 208350.0f ) {
                            return 106.0/454.3;
                        } else {
                            if ( cl->stats.sum_uip1_used <= 53.5f ) {
                                if ( rdb0_last_touched_diff <= 3189.5f ) {
                                    return 34.0/402.2;
                                } else {
                                    return 69.0/368.2;
                                }
                            } else {
                                if ( cl->stats.size_rel <= 0.481072604656f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.113117821515f ) {
                                        return 33.0/328.2;
                                    } else {
                                        return 30.0/440.2;
                                    }
                                } else {
                                    return 19.0/586.3;
                                }
                            }
                        }
                    } else {
                        return 88.0/416.2;
                    }
                } else {
                    if ( cl->stats.antec_num_total_lits_rel <= 0.325178563595f ) {
                        if ( rdb0_last_touched_diff <= 2552.0f ) {
                            if ( cl->stats.sum_uip1_used <= 245.5f ) {
                                if ( rdb0_last_touched_diff <= 754.5f ) {
                                    if ( rdb0_last_touched_diff <= 255.0f ) {
                                        return 28.0/424.2;
                                    } else {
                                        return 9.0/436.2;
                                    }
                                } else {
                                    return 40.0/584.3;
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.686329722404f ) {
                                    return 1.0/448.3;
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 52.5f ) {
                                        return 4.0/498.3;
                                    } else {
                                        return 7.0/408.2;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 17.5f ) {
                                return 46.0/474.3;
                            } else {
                                return 20.0/408.2;
                            }
                        }
                    } else {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 37010172.0f ) {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 5587045.0f ) {
                                return 11.0/542.3;
                            } else {
                                return 22.0/394.2;
                            }
                        } else {
                            return 4.0/590.3;
                        }
                    }
                }
            }
        }
    }
}

static double estimator_should_keep_long_conf2_cluster0_3(
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
            if ( cl->stats.size_rel <= 0.631822705269f ) {
                if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                    if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                        if ( cl->stats.glue_rel_long <= 0.813189148903f ) {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 6474103.0f ) {
                                if ( rdb0_last_touched_diff <= 26572.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.187987908721f ) {
                                        if ( cl->size() <= 16.5f ) {
                                            if ( cl->stats.sum_uip1_used <= 17.5f ) {
                                                if ( cl->stats.dump_number <= 14.5f ) {
                                                    if ( cl->stats.glue_rel_queue <= 0.286060065031f ) {
                                                        return 84.0/238.1;
                                                    } else {
                                                        if ( cl->stats.num_total_lits_antecedents <= 27.5f ) {
                                                            if ( cl->stats.size_rel <= 0.290614843369f ) {
                                                                if ( cl->stats.sum_delta_confl_uip1_used <= 87223.5f ) {
                                                                    return 71.0/374.2;
                                                                } else {
                                                                    return 49.0/382.2;
                                                                }
                                                            } else {
                                                                return 98.0/382.2;
                                                            }
                                                        } else {
                                                            return 81.0/276.2;
                                                        }
                                                    }
                                                } else {
                                                    return 182.0/194.1;
                                                }
                                            } else {
                                                if ( cl->stats.dump_number <= 24.5f ) {
                                                    if ( cl->stats.num_total_lits_antecedents <= 18.5f ) {
                                                        if ( cl->stats.num_total_lits_antecedents <= 13.5f ) {
                                                            return 80.0/590.3;
                                                        } else {
                                                            return 37.0/462.3;
                                                        }
                                                    } else {
                                                        return 81.0/516.3;
                                                    }
                                                } else {
                                                    return 85.0/230.1;
                                                }
                                            }
                                        } else {
                                            return 201.0/376.2;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.697853326797f ) {
                                            if ( rdb0_last_touched_diff <= 4830.5f ) {
                                                return 94.0/370.2;
                                            } else {
                                                if ( cl->size() <= 9.5f ) {
                                                    return 98.0/346.2;
                                                } else {
                                                    return 172.0/292.2;
                                                }
                                            }
                                        } else {
                                            return 111.0/222.1;
                                        }
                                    }
                                } else {
                                    if ( cl->size() <= 10.5f ) {
                                        if ( cl->stats.glue_rel_long <= 0.322351157665f ) {
                                            return 126.0/280.2;
                                        } else {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.160891592503f ) {
                                                if ( cl->stats.sum_uip1_used <= 11.5f ) {
                                                    return 92.0/288.2;
                                                } else {
                                                    return 63.0/300.2;
                                                }
                                            } else {
                                                return 94.0/248.1;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 900026.0f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0900107771158f ) {
                                                return 163.0/208.1;
                                            } else {
                                                return 140.0/268.2;
                                            }
                                        } else {
                                            return 98.0/246.1;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 174730144.0f ) {
                                    if ( cl->stats.sum_uip1_used <= 54.5f ) {
                                        return 128.0/236.1;
                                    } else {
                                        if ( cl->stats.size_rel <= 0.094066567719f ) {
                                            return 27.0/418.2;
                                        } else {
                                            if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                                                if ( cl->stats.dump_number <= 68.5f ) {
                                                    if ( cl->stats.glue_rel_queue <= 0.442024290562f ) {
                                                        return 101.0/598.3;
                                                    } else {
                                                        return 49.0/554.3;
                                                    }
                                                } else {
                                                    return 90.0/230.1;
                                                }
                                            } else {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.0680621266365f ) {
                                                    return 55.0/348.2;
                                                } else {
                                                    if ( cl->stats.sum_uip1_used <= 165.5f ) {
                                                        return 56.0/404.2;
                                                    } else {
                                                        return 21.0/478.3;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    return 47.0/652.4;
                                }
                            }
                        } else {
                            if ( cl->size() <= 10.5f ) {
                                if ( cl->stats.sum_uip1_used <= 10.5f ) {
                                    return 118.0/252.1;
                                } else {
                                    return 65.0/370.2;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.69882786274f ) {
                                    if ( cl->stats.sum_uip1_used <= 8.5f ) {
                                        return 288.0/194.1;
                                    } else {
                                        return 121.0/338.2;
                                    }
                                } else {
                                    return 191.0/54.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_queue <= 1.00840616226f ) {
                            if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                if ( cl->stats.size_rel <= 0.100736647844f ) {
                                    return 31.0/394.2;
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 17.5f ) {
                                        if ( rdb0_last_touched_diff <= 2270.5f ) {
                                            return 77.0/404.2;
                                        } else {
                                            return 52.0/628.4;
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 27.5f ) {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.156385689974f ) {
                                                return 133.0/498.3;
                                            } else {
                                                return 77.0/462.3;
                                            }
                                        } else {
                                            return 49.0/338.2;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.0202689170837f ) {
                                    return 49.0/320.2;
                                } else {
                                    if ( cl->size() <= 8.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 41055.0f ) {
                                            if ( cl->stats.num_total_lits_antecedents <= 19.5f ) {
                                                if ( cl->stats.used_for_uip_creation <= 13.5f ) {
                                                    if ( cl->stats.rdb1_last_touched_diff <= 8193.0f ) {
                                                        if ( cl->stats.glue_rel_queue <= 0.355468302965f ) {
                                                            return 14.0/420.2;
                                                        } else {
                                                            return 37.0/630.4;
                                                        }
                                                    } else {
                                                        if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                                            return 21.0/452.3;
                                                        } else {
                                                            return 41.0/470.3;
                                                        }
                                                    }
                                                } else {
                                                    if ( cl->stats.rdb1_last_touched_diff <= 11295.0f ) {
                                                        return 23.0/654.4;
                                                    } else {
                                                        return 8.0/436.2;
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.num_overlap_literals_rel <= 0.128018796444f ) {
                                                    return 37.0/400.2;
                                                } else {
                                                    return 23.0/538.3;
                                                }
                                            }
                                        } else {
                                            return 39.0/366.2;
                                        }
                                    } else {
                                        if ( cl->stats.used_for_uip_creation <= 30.5f ) {
                                            if ( rdb0_last_touched_diff <= 802.0f ) {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.138100087643f ) {
                                                    return 38.0/366.2;
                                                } else {
                                                    return 28.0/460.3;
                                                }
                                            } else {
                                                if ( cl->stats.sum_uip1_used <= 105.5f ) {
                                                    if ( cl->stats.dump_number <= 14.5f ) {
                                                        if ( cl->stats.glue_rel_long <= 0.526913523674f ) {
                                                            return 31.0/390.2;
                                                        } else {
                                                            return 58.0/340.2;
                                                        }
                                                    } else {
                                                        return 80.0/280.2;
                                                    }
                                                } else {
                                                    return 27.0/430.2;
                                                }
                                            }
                                        } else {
                                            return 11.0/424.2;
                                        }
                                    }
                                }
                            }
                        } else {
                            return 57.0/330.2;
                        }
                    }
                } else {
                    if ( cl->stats.dump_number <= 1.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 2081.5f ) {
                            if ( cl->stats.used_for_uip_creation <= 5.5f ) {
                                return 56.0/386.2;
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 32.5f ) {
                                    if ( cl->stats.glue <= 4.5f ) {
                                        return 5.0/454.3;
                                    } else {
                                        return 11.0/416.2;
                                    }
                                } else {
                                    return 16.0/428.2;
                                }
                            }
                        } else {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 44738.5f ) {
                                return 76.0/280.2;
                            } else {
                                return 45.0/502.3;
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 4926.5f ) {
                            if ( cl->stats.sum_uip1_used <= 81.5f ) {
                                if ( cl->stats.dump_number <= 8.5f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 5.5f ) {
                                        return 51.0/552.3;
                                    } else {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 8.5f ) {
                                            if ( cl->stats.used_for_uip_creation <= 6.5f ) {
                                                return 16.0/386.2;
                                            } else {
                                                return 29.0/380.2;
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 0.616482973099f ) {
                                                if ( cl->size() <= 5.5f ) {
                                                    return 7.0/596.3;
                                                } else {
                                                    if ( rdb0_last_touched_diff <= 1049.0f ) {
                                                        return 10.0/380.2;
                                                    } else {
                                                        return 23.0/440.2;
                                                    }
                                                }
                                            } else {
                                                return 22.0/384.2;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0563875287771f ) {
                                        return 67.0/492.3;
                                    } else {
                                        return 42.0/554.3;
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_uip1_used <= 253.5f ) {
                                    if ( rdb0_last_touched_diff <= 3418.0f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0243741068989f ) {
                                            return 7.0/804.5;
                                        } else {
                                            if ( cl->stats.glue <= 7.5f ) {
                                                if ( cl->stats.used_for_uip_creation <= 5.5f ) {
                                                    return 35.0/726.4;
                                                } else {
                                                    if ( cl->stats.glue_rel_queue <= 0.221919953823f ) {
                                                        return 19.0/378.2;
                                                    } else {
                                                        if ( cl->stats.used_for_uip_creation <= 17.5f ) {
                                                            if ( rdb0_last_touched_diff <= 1508.5f ) {
                                                                if ( cl->stats.glue_rel_long <= 0.401750683784f ) {
                                                                    return 0.0/410.2;
                                                                } else {
                                                                    return 3.0/392.2;
                                                                }
                                                            } else {
                                                                return 7.0/398.2;
                                                            }
                                                        } else {
                                                            if ( cl->stats.size_rel <= 0.301327586174f ) {
                                                                if ( cl->stats.used_for_uip_creation <= 34.5f ) {
                                                                    return 2.0/508.3;
                                                                } else {
                                                                    return 10.0/446.3;
                                                                }
                                                            } else {
                                                                return 21.0/394.2;
                                                            }
                                                        }
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.num_antecedents_rel <= 0.142694309354f ) {
                                                    return 37.0/440.2;
                                                } else {
                                                    return 23.0/628.4;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 17.5f ) {
                                            return 14.0/378.2;
                                        } else {
                                            return 33.0/342.2;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.044848240912f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0122166592628f ) {
                                            if ( cl->stats.sum_uip1_used <= 434.5f ) {
                                                return 19.0/476.3;
                                            } else {
                                                if ( cl->stats.num_antecedents_rel <= 0.0172296278179f ) {
                                                    return 8.0/634.4;
                                                } else {
                                                    return 2.0/438.2;
                                                }
                                            }
                                        } else {
                                            return 33.0/664.4;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 43.5f ) {
                                            if ( cl->stats.num_total_lits_antecedents <= 28.5f ) {
                                                if ( cl->stats.used_for_uip_creation <= 57.5f ) {
                                                    if ( cl->stats.glue_rel_queue <= 0.241479456425f ) {
                                                        return 11.0/504.3;
                                                    } else {
                                                        if ( cl->stats.glue_rel_queue <= 0.319625675678f ) {
                                                            return 0.0/552.3;
                                                        } else {
                                                            if ( cl->stats.size_rel <= 0.334041237831f ) {
                                                                if ( cl->stats.sum_delta_confl_uip1_used <= 80994800.0f ) {
                                                                    return 3.0/636.4;
                                                                } else {
                                                                    return 13.0/678.4;
                                                                }
                                                            } else {
                                                                return 1.0/444.3;
                                                            }
                                                        }
                                                    }
                                                } else {
                                                    return 12.0/428.2;
                                                }
                                            } else {
                                                if ( cl->stats.glue_rel_long <= 0.343279749155f ) {
                                                    return 21.0/388.2;
                                                } else {
                                                    if ( cl->stats.num_total_lits_antecedents <= 63.5f ) {
                                                        return 13.0/468.3;
                                                    } else {
                                                        return 5.0/402.2;
                                                    }
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.173749998212f ) {
                                                if ( cl->size() <= 11.5f ) {
                                                    if ( cl->stats.glue_rel_long <= 0.313902467489f ) {
                                                        return 9.0/746.4;
                                                    } else {
                                                        return 0.0/674.4;
                                                    }
                                                } else {
                                                    return 1.0/842.5;
                                                }
                                            } else {
                                                return 17.0/650.4;
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 11.5f ) {
                                if ( cl->stats.glue_rel_queue <= 0.663423418999f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0181331504136f ) {
                                        return 77.0/356.2;
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.515239477158f ) {
                                            if ( cl->stats.sum_uip1_used <= 35.5f ) {
                                                return 54.0/356.2;
                                            } else {
                                                if ( cl->stats.num_antecedents_rel <= 0.149874642491f ) {
                                                    return 27.0/640.4;
                                                } else {
                                                    return 43.0/642.4;
                                                }
                                            }
                                        } else {
                                            return 53.0/488.3;
                                        }
                                    }
                                } else {
                                    return 104.0/528.3;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 25.5f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0688373222947f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.107684001327f ) {
                                            if ( cl->stats.glue_rel_long <= 0.331048190594f ) {
                                                return 34.0/398.2;
                                            } else {
                                                return 18.0/580.3;
                                            }
                                        } else {
                                            return 49.0/530.3;
                                        }
                                    } else {
                                        return 22.0/728.4;
                                    }
                                } else {
                                    return 33.0/382.2;
                                }
                            }
                        }
                    }
                }
            } else {
                if ( rdb0_last_touched_diff <= 9871.5f ) {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 796848.0f ) {
                            if ( rdb0_last_touched_diff <= 2830.0f ) {
                                return 162.0/414.2;
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 13117.5f ) {
                                    return 102.0/216.1;
                                } else {
                                    return 202.0/134.1;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 50262.0f ) {
                                if ( cl->stats.sum_uip1_used <= 37.5f ) {
                                    return 141.0/324.2;
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.074046626687f ) {
                                        return 29.0/434.2;
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.403883844614f ) {
                                            return 57.0/368.2;
                                        } else {
                                            return 68.0/310.2;
                                        }
                                    }
                                }
                            } else {
                                return 96.0/280.2;
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 5263.5f ) {
                            if ( cl->stats.sum_uip1_used <= 32.5f ) {
                                return 62.0/268.2;
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.205373466015f ) {
                                    return 24.0/370.2;
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.324999988079f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 993.5f ) {
                                            return 2.0/678.4;
                                        } else {
                                            return 13.0/522.3;
                                        }
                                    } else {
                                        return 36.0/742.4;
                                    }
                                }
                            }
                        } else {
                            return 57.0/374.2;
                        }
                    }
                } else {
                    if ( cl->stats.num_overlap_literals_rel <= 0.294312894344f ) {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 3211.0f ) {
                            return 313.0/196.1;
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 1.64374995232f ) {
                                if ( cl->size() <= 28.5f ) {
                                    return 136.0/312.2;
                                } else {
                                    return 93.0/302.2;
                                }
                            } else {
                                if ( cl->stats.sum_uip1_used <= 15.5f ) {
                                    return 143.0/126.1;
                                } else {
                                    return 93.0/246.1;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.sum_uip1_used <= 9.5f ) {
                            if ( rdb0_last_touched_diff <= 29171.0f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 16.2916679382f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 129.5f ) {
                                        return 159.0/110.1;
                                    } else {
                                        return 202.0/56.0;
                                    }
                                } else {
                                    return 185.0/46.0;
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.972433686256f ) {
                                    return 280.0/28.0;
                                } else {
                                    return 222.0/38.0;
                                }
                            }
                        } else {
                            return 164.0/326.2;
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.sum_uip1_used <= 6.5f ) {
                if ( cl->stats.antec_num_total_lits_rel <= 0.307657957077f ) {
                    if ( cl->stats.num_total_lits_antecedents <= 36.5f ) {
                        if ( cl->size() <= 8.5f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                return 116.0/362.2;
                            } else {
                                return 147.0/278.2;
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals <= 7.5f ) {
                                return 241.0/174.1;
                            } else {
                                return 219.0/306.2;
                            }
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.364197552204f ) {
                            return 169.0/158.1;
                        } else {
                            if ( cl->stats.num_overlap_literals_rel <= 0.133716985583f ) {
                                return 200.0/62.0;
                            } else {
                                return 180.0/72.0;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_total_lits_antecedents <= 68.5f ) {
                        if ( cl->stats.antec_num_total_lits_rel <= 0.409211456776f ) {
                            return 195.0/100.1;
                        } else {
                            return 136.0/160.1;
                        }
                    } else {
                        if ( cl->stats.glue_rel_long <= 0.667828083038f ) {
                            return 189.0/138.1;
                        } else {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 17.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 1.83526873589f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 15.6184368134f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.741148591042f ) {
                                            return 220.0/2.0;
                                        } else {
                                            return 323.0/34.0;
                                        }
                                    } else {
                                        return 1;
                                    }
                                } else {
                                    return 235.0/32.0;
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.992754399776f ) {
                                    return 223.0/94.1;
                                } else {
                                    return 313.0/76.0;
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.sum_uip1_used <= 44.5f ) {
                    if ( rdb0_last_touched_diff <= 21243.0f ) {
                        if ( cl->stats.size_rel <= 0.706403374672f ) {
                            if ( cl->stats.glue_rel_queue <= 0.445034772158f ) {
                                return 151.0/908.5;
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0414250642061f ) {
                                    return 110.0/212.1;
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.19091796875f ) {
                                        return 113.0/512.3;
                                    } else {
                                        return 181.0/454.3;
                                    }
                                }
                            }
                        } else {
                            return 196.0/284.2;
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 37.5f ) {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 838201.0f ) {
                                if ( cl->stats.dump_number <= 9.5f ) {
                                    return 123.0/344.2;
                                } else {
                                    return 132.0/260.1;
                                }
                            } else {
                                if ( cl->size() <= 9.5f ) {
                                    return 201.0/282.2;
                                } else {
                                    return 165.0/128.1;
                                }
                            }
                        } else {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 221056.0f ) {
                                return 232.0/158.1;
                            } else {
                                if ( cl->stats.sum_uip1_used <= 18.5f ) {
                                    return 187.0/166.1;
                                } else {
                                    return 141.0/224.1;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 52857928.0f ) {
                                    if ( cl->stats.glue <= 6.5f ) {
                                        return 120.0/530.3;
                                    } else {
                                        return 84.0/260.1;
                                    }
                                } else {
                                    return 34.0/364.2;
                                }
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 45181720.0f ) {
                                    if ( cl->stats.num_overlap_literals <= 15.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.466563105583f ) {
                                            return 124.0/448.3;
                                        } else {
                                            return 125.0/284.2;
                                        }
                                    } else {
                                        return 115.0/236.1;
                                    }
                                } else {
                                    return 81.0/530.3;
                                }
                            }
                        } else {
                            if ( cl->stats.dump_number <= 15.5f ) {
                                return 22.0/402.2;
                            } else {
                                return 50.0/386.2;
                            }
                        }
                    } else {
                        if ( cl->stats.num_overlap_literals <= 5.5f ) {
                            return 31.0/742.4;
                        } else {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.0796178132296f ) {
                                return 44.0/322.2;
                            } else {
                                if ( rdb0_last_touched_diff <= 6180.0f ) {
                                    return 16.0/442.2;
                                } else {
                                    return 40.0/326.2;
                                }
                            }
                        }
                    }
                }
            }
        }
    } else {
        if ( cl->stats.size_rel <= 0.53054523468f ) {
            if ( cl->stats.sum_delta_confl_uip1_used <= 1738.0f ) {
                if ( cl->stats.glue_rel_long <= 0.716827511787f ) {
                    if ( cl->stats.rdb1_last_touched_diff <= 79980.0f ) {
                        if ( cl->stats.glue <= 5.5f ) {
                            if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                if ( cl->stats.glue_rel_queue <= 0.462250709534f ) {
                                    return 93.0/242.1;
                                } else {
                                    return 120.0/194.1;
                                }
                            } else {
                                return 146.0/200.1;
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.553706407547f ) {
                                return 229.0/328.2;
                            } else {
                                return 273.0/204.1;
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 183720.0f ) {
                            if ( cl->stats.size_rel <= 0.184058278799f ) {
                                return 140.0/134.1;
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.206833973527f ) {
                                    return 230.0/100.1;
                                } else {
                                    return 144.0/134.1;
                                }
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 39.5f ) {
                                return 324.0/168.1;
                            } else {
                                return 233.0/76.0;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.antecedents_glue_long_reds_var <= 10.0019130707f ) {
                        if ( cl->stats.glue_rel_long <= 1.00692534447f ) {
                            if ( cl->stats.size_rel <= 0.238086700439f ) {
                                return 266.0/224.1;
                            } else {
                                if ( cl->size() <= 10.5f ) {
                                    return 256.0/154.1;
                                } else {
                                    if ( rdb0_last_touched_diff <= 78613.5f ) {
                                        return 207.0/94.1;
                                    } else {
                                        return 279.0/48.0;
                                    }
                                }
                            }
                        } else {
                            if ( cl->size() <= 12.5f ) {
                                return 194.0/98.1;
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 97866.5f ) {
                                    return 238.0/78.0;
                                } else {
                                    return 193.0/14.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 36.5f ) {
                            if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 33076.0f ) {
                                    return 198.0/10.0;
                                } else {
                                    return 363.0/2.0;
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 7.5f ) {
                                    return 199.0/44.0;
                                } else {
                                    return 208.0/18.0;
                                }
                            }
                        } else {
                            return 196.0/42.0;
                        }
                    }
                }
            } else {
                if ( cl->stats.rdb1_last_touched_diff <= 45116.5f ) {
                    if ( cl->stats.sum_uip1_used <= 20.5f ) {
                        if ( cl->stats.size_rel <= 0.426099717617f ) {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 90363.5f ) {
                                if ( cl->stats.glue <= 6.5f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                                        return 143.0/418.2;
                                    } else {
                                        return 70.0/338.2;
                                    }
                                } else {
                                    if ( cl->size() <= 16.5f ) {
                                        return 117.0/198.1;
                                    } else {
                                        return 96.0/212.1;
                                    }
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.0643660202622f ) {
                                    return 155.0/122.1;
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.162371024489f ) {
                                        return 116.0/274.2;
                                    } else {
                                        return 163.0/240.1;
                                    }
                                }
                            }
                        } else {
                            return 245.0/290.2;
                        }
                    } else {
                        if ( cl->stats.dump_number <= 20.5f ) {
                            if ( cl->stats.glue_rel_long <= 0.696117460728f ) {
                                if ( cl->stats.sum_uip1_used <= 98.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.0679873079062f ) {
                                        return 71.0/314.2;
                                    } else {
                                        if ( rdb0_last_touched_diff <= 32334.5f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0800330638885f ) {
                                                return 34.0/378.2;
                                            } else {
                                                return 41.0/356.2;
                                            }
                                        } else {
                                            return 50.0/354.2;
                                        }
                                    }
                                } else {
                                    return 25.0/698.4;
                                }
                            } else {
                                return 92.0/344.2;
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 83.5f ) {
                                return 141.0/184.1;
                            } else {
                                return 64.0/318.2;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.sum_delta_confl_uip1_used <= 6791265.0f ) {
                        if ( cl->size() <= 10.5f ) {
                            if ( cl->stats.glue_rel_long <= 0.478531271219f ) {
                                if ( cl->stats.dump_number <= 25.5f ) {
                                    if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                        return 187.0/264.1;
                                    } else {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 99344.0f ) {
                                            return 120.0/258.1;
                                        } else {
                                            return 132.0/458.3;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 9.5f ) {
                                        return 177.0/90.1;
                                    } else {
                                        if ( rdb0_last_touched_diff <= 183944.5f ) {
                                            return 172.0/122.1;
                                        } else {
                                            return 160.0/226.1;
                                        }
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 182954.5f ) {
                                    if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                        if ( cl->stats.dump_number <= 21.5f ) {
                                            return 194.0/374.2;
                                        } else {
                                            return 158.0/102.1;
                                        }
                                    } else {
                                        return 245.0/280.2;
                                    }
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 11.5f ) {
                                        return 210.0/112.1;
                                    } else {
                                        return 150.0/130.1;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 12.5f ) {
                                if ( cl->stats.sum_uip1_used <= 2.5f ) {
                                    return 165.0/136.1;
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.494030535221f ) {
                                        return 220.0/154.1;
                                    } else {
                                        if ( cl->size() <= 13.5f ) {
                                            return 241.0/40.0;
                                        } else {
                                            if ( cl->stats.num_overlap_literals <= 27.5f ) {
                                                return 182.0/90.1;
                                            } else {
                                                return 190.0/60.0;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.dump_number <= 27.5f ) {
                                    if ( cl->stats.num_overlap_literals <= 20.5f ) {
                                        return 151.0/306.2;
                                    } else {
                                        return 137.0/154.1;
                                    }
                                } else {
                                    return 235.0/200.1;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 39396080.0f ) {
                            if ( cl->stats.glue <= 5.5f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 14538344.0f ) {
                                    return 130.0/264.1;
                                } else {
                                    return 103.0/256.1;
                                }
                            } else {
                                return 242.0/330.2;
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 144168.0f ) {
                                return 41.0/348.2;
                            } else {
                                return 109.0/232.1;
                            }
                        }
                    }
                }
            }
        } else {
            if ( rdb0_last_touched_diff <= 47927.5f ) {
                if ( cl->stats.dump_number <= 4.5f ) {
                    if ( cl->stats.glue <= 10.5f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 56.5f ) {
                            if ( cl->size() <= 13.5f ) {
                                return 139.0/242.1;
                            } else {
                                return 163.0/110.1;
                            }
                        } else {
                            if ( cl->stats.glue <= 8.5f ) {
                                return 263.0/152.1;
                            } else {
                                return 260.0/106.1;
                            }
                        }
                    } else {
                        if ( cl->stats.antec_num_total_lits_rel <= 0.972036361694f ) {
                            if ( cl->stats.sum_uip1_used <= 3.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.305000007153f ) {
                                    return 183.0/130.1;
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 1.04429137707f ) {
                                        return 341.0/122.1;
                                    } else {
                                        if ( cl->size() <= 50.5f ) {
                                            return 241.0/56.0;
                                        } else {
                                            return 267.0/32.0;
                                        }
                                    }
                                }
                            } else {
                                return 143.0/244.1;
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals <= 326.5f ) {
                                if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                    return 316.0/44.0;
                                } else {
                                    return 185.0/46.0;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 3.04044651985f ) {
                                    return 292.0/14.0;
                                } else {
                                    return 200.0/24.0;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.sum_uip1_used <= 29.5f ) {
                        if ( cl->size() <= 52.0f ) {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.332908630371f ) {
                                return 183.0/124.1;
                            } else {
                                return 171.0/212.1;
                            }
                        } else {
                            return 186.0/92.1;
                        }
                    } else {
                        return 111.0/494.3;
                    }
                }
            } else {
                if ( cl->stats.sum_uip1_used <= 17.5f ) {
                    if ( cl->stats.glue <= 13.5f ) {
                        if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                            if ( cl->size() <= 11.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.254420489073f ) {
                                    return 201.0/106.1;
                                } else {
                                    return 191.0/138.1;
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                    if ( cl->stats.size_rel <= 0.825153946877f ) {
                                        if ( cl->stats.sum_uip1_used <= 4.5f ) {
                                            if ( rdb0_last_touched_diff <= 250083.0f ) {
                                                if ( cl->stats.glue <= 8.5f ) {
                                                    return 259.0/130.1;
                                                } else {
                                                    if ( cl->stats.sum_delta_confl_uip1_used <= 5.5f ) {
                                                        return 304.0/32.0;
                                                    } else {
                                                        return 228.0/86.0;
                                                    }
                                                }
                                            } else {
                                                return 232.0/28.0;
                                            }
                                        } else {
                                            if ( cl->stats.size_rel <= 0.666525483131f ) {
                                                return 150.0/82.0;
                                            } else {
                                                return 151.0/106.1;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 39.5f ) {
                                            return 225.0/76.0;
                                        } else {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.226466059685f ) {
                                                return 193.0/52.0;
                                            } else {
                                                if ( rdb0_last_touched_diff <= 320548.5f ) {
                                                    if ( cl->stats.size_rel <= 1.63962101936f ) {
                                                        if ( cl->stats.sum_uip1_used <= 4.5f ) {
                                                            if ( cl->stats.size_rel <= 1.33791005611f ) {
                                                                if ( cl->stats.antec_num_total_lits_rel <= 0.587741613388f ) {
                                                                    return 362.0/8.0;
                                                                } else {
                                                                    if ( cl->stats.glue_rel_long <= 0.924409747124f ) {
                                                                        return 185.0/24.0;
                                                                    } else {
                                                                        return 283.0/10.0;
                                                                    }
                                                                }
                                                            } else {
                                                                return 201.0/36.0;
                                                            }
                                                        } else {
                                                            return 198.0/58.0;
                                                        }
                                                    } else {
                                                        return 236.0/2.0;
                                                    }
                                                } else {
                                                    return 197.0/46.0;
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.389187157154f ) {
                                        if ( rdb0_act_ranking_top_10 <= 7.5f ) {
                                            return 321.0/130.1;
                                        } else {
                                            return 376.0/96.1;
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 206371.5f ) {
                                            return 296.0/190.1;
                                        } else {
                                            return 314.0/132.1;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.glue <= 7.5f ) {
                                return 313.0/98.1;
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.126219987869f ) {
                                    return 184.0/48.0;
                                } else {
                                    if ( cl->stats.dump_number <= 18.5f ) {
                                        if ( cl->size() <= 23.5f ) {
                                            return 224.0/22.0;
                                        } else {
                                            return 173.0/50.0;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.894470572472f ) {
                                            return 257.0/28.0;
                                        } else {
                                            return 315.0/8.0;
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.243966937065f ) {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.241088032722f ) {
                                return 196.0/26.0;
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.163261562586f ) {
                                    return 253.0/150.1;
                                } else {
                                    return 303.0/62.0;
                                }
                            }
                        } else {
                            if ( cl->stats.num_antecedents_rel <= 4.00704956055f ) {
                                if ( cl->stats.size_rel <= 0.726984739304f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 232.5f ) {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 20.6913261414f ) {
                                            return 181.0/60.0;
                                        } else {
                                            return 191.0/48.0;
                                        }
                                    } else {
                                        return 333.0/24.0;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 517244.0f ) {
                                        if ( cl->stats.rdb1_act_ranking_top_10 <= 5.5f ) {
                                            if ( cl->stats.sum_uip1_used <= 4.5f ) {
                                                if ( cl->size() <= 30.5f ) {
                                                    if ( cl->stats.sum_delta_confl_uip1_used <= 4.5f ) {
                                                        return 325.0/10.0;
                                                    } else {
                                                        return 202.0/30.0;
                                                    }
                                                } else {
                                                    if ( cl->stats.rdb1_last_touched_diff <= 98272.0f ) {
                                                        if ( cl->stats.antecedents_glue_long_reds_var <= 63.104019165f ) {
                                                            return 374.0/34.0;
                                                        } else {
                                                            return 213.0/2.0;
                                                        }
                                                    } else {
                                                        if ( cl->stats.glue_rel_long <= 1.03863608837f ) {
                                                            return 203.0/4.0;
                                                        } else {
                                                            return 1;
                                                        }
                                                    }
                                                }
                                            } else {
                                                return 191.0/36.0;
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_act_ranking_top_10 <= 8.5f ) {
                                                if ( cl->stats.num_antecedents_rel <= 1.72738075256f ) {
                                                    if ( cl->stats.rdb1_last_touched_diff <= 103670.5f ) {
                                                        if ( rdb0_act_ranking_top_10 <= 7.5f ) {
                                                            if ( cl->stats.size_rel <= 1.40935850143f ) {
                                                                return 237.0/26.0;
                                                            } else {
                                                                return 183.0/50.0;
                                                            }
                                                        } else {
                                                            return 233.0/20.0;
                                                        }
                                                    } else {
                                                        if ( cl->stats.sum_delta_confl_uip1_used <= 36.5f ) {
                                                            if ( cl->stats.num_total_lits_antecedents <= 283.5f ) {
                                                                if ( cl->stats.sum_delta_confl_uip1_used <= 1.5f ) {
                                                                    return 213.0/30.0;
                                                                } else {
                                                                    return 256.0/8.0;
                                                                }
                                                            } else {
                                                                return 1;
                                                            }
                                                        } else {
                                                            return 199.0/30.0;
                                                        }
                                                    }
                                                } else {
                                                    return 280.0/60.0;
                                                }
                                            } else {
                                                if ( cl->size() <= 150.0f ) {
                                                    if ( rdb0_last_touched_diff <= 161588.0f ) {
                                                        return 289.0/22.0;
                                                    } else {
                                                        if ( cl->stats.size_rel <= 1.17588019371f ) {
                                                            return 200.0/10.0;
                                                        } else {
                                                            return 360.0/2.0;
                                                        }
                                                    }
                                                } else {
                                                    return 269.0/30.0;
                                                }
                                            }
                                        }
                                    } else {
                                        return 249.0/72.0;
                                    }
                                }
                            } else {
                                return 249.0/60.0;
                            }
                        }
                    }
                } else {
                    if ( rdb0_last_touched_diff <= 93594.0f ) {
                        if ( rdb0_last_touched_diff <= 68300.0f ) {
                            return 104.0/192.1;
                        } else {
                            return 88.0/238.1;
                        }
                    } else {
                        if ( cl->stats.num_overlap_literals_rel <= 0.231071710587f ) {
                            if ( rdb0_last_touched_diff <= 231125.0f ) {
                                return 235.0/346.2;
                            } else {
                                return 176.0/152.1;
                            }
                        } else {
                            return 225.0/212.1;
                        }
                    }
                }
            }
        }
    }
}

static double estimator_should_keep_long_conf2_cluster0_4(
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
        if ( cl->stats.glue_rel_queue <= 0.719455242157f ) {
            if ( cl->size() <= 8.5f ) {
                if ( rdb0_last_touched_diff <= 10327.5f ) {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 6685045.0f ) {
                            if ( cl->stats.dump_number <= 14.5f ) {
                                if ( cl->stats.sum_uip1_used <= 7.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 9660.5f ) {
                                        return 73.0/352.2;
                                    } else {
                                        return 99.0/318.2;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.203639179468f ) {
                                        return 63.0/318.2;
                                    } else {
                                        if ( cl->stats.dump_number <= 7.5f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0431313589215f ) {
                                                return 60.0/710.4;
                                            } else {
                                                if ( cl->stats.num_total_lits_antecedents <= 17.5f ) {
                                                    return 12.0/478.3;
                                                } else {
                                                    return 33.0/464.3;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.0722455084324f ) {
                                                return 56.0/328.2;
                                            } else {
                                                return 59.0/606.3;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 3155506.5f ) {
                                    return 173.0/366.2;
                                } else {
                                    return 76.0/460.3;
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 955.5f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.0961385369301f ) {
                                    return 39.0/584.3;
                                } else {
                                    return 15.0/476.3;
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.220590144396f ) {
                                    return 53.0/378.2;
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                        if ( cl->stats.dump_number <= 51.5f ) {
                                            return 43.0/518.3;
                                        } else {
                                            return 64.0/356.2;
                                        }
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.065961971879f ) {
                                            return 29.0/384.2;
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 8707.5f ) {
                                                return 11.0/430.2;
                                            } else {
                                                return 29.0/428.2;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 17.5f ) {
                            if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                if ( cl->stats.glue <= 3.5f ) {
                                    if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                                        return 38.0/408.2;
                                    } else {
                                        return 46.0/482.3;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.443233221769f ) {
                                        if ( cl->stats.num_overlap_literals <= 3.5f ) {
                                            return 9.0/422.2;
                                        } else {
                                            return 23.0/534.3;
                                        }
                                    } else {
                                        return 30.0/562.3;
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 1156.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 10.5f ) {
                                        if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                            if ( cl->stats.sum_uip1_used <= 255.0f ) {
                                                return 18.0/452.3;
                                            } else {
                                                return 6.0/628.4;
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 17.5f ) {
                                                return 2.0/778.4;
                                            } else {
                                                return 5.0/454.3;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.0933826118708f ) {
                                            if ( cl->stats.dump_number <= 6.5f ) {
                                                return 27.0/470.3;
                                            } else {
                                                if ( cl->stats.rdb1_used_for_uip_creation <= 26.5f ) {
                                                    return 20.0/430.2;
                                                } else {
                                                    return 6.0/412.2;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.196139782667f ) {
                                                if ( cl->stats.sum_uip1_used <= 170.0f ) {
                                                    return 7.0/398.2;
                                                } else {
                                                    return 0.0/614.3;
                                                }
                                            } else {
                                                return 15.0/382.2;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 1.5f ) {
                                        return 25.0/382.2;
                                    } else {
                                        if ( cl->stats.sum_uip1_used <= 234.5f ) {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 16043222.0f ) {
                                                if ( cl->stats.glue_rel_queue <= 0.360288470984f ) {
                                                    if ( cl->stats.dump_number <= 6.5f ) {
                                                        return 15.0/486.3;
                                                    } else {
                                                        return 24.0/494.3;
                                                    }
                                                } else {
                                                    if ( cl->stats.num_total_lits_antecedents <= 11.5f ) {
                                                        return 13.0/484.3;
                                                    } else {
                                                        return 7.0/550.3;
                                                    }
                                                }
                                            } else {
                                                return 37.0/348.2;
                                            }
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.0797442868352f ) {
                                                if ( cl->stats.num_overlap_literals_rel <= 0.0123329237103f ) {
                                                    return 2.0/404.2;
                                                } else {
                                                    return 21.0/442.2;
                                                }
                                            } else {
                                                if ( cl->size() <= 4.5f ) {
                                                    return 3.0/500.3;
                                                } else {
                                                    return 6.0/494.3;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0924066007137f ) {
                                    return 57.0/374.2;
                                } else {
                                    return 38.0/488.3;
                                }
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 31.5f ) {
                                    if ( cl->stats.glue <= 4.5f ) {
                                        if ( cl->stats.size_rel <= 0.150622099638f ) {
                                            if ( cl->stats.sum_uip1_used <= 101.5f ) {
                                                return 9.0/368.2;
                                            } else {
                                                return 7.0/440.2;
                                            }
                                        } else {
                                            return 41.0/558.3;
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 1699.0f ) {
                                            return 19.0/526.3;
                                        } else {
                                            return 50.0/600.3;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 6292129.0f ) {
                                        return 12.0/378.2;
                                    } else {
                                        return 6.0/772.4;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                        if ( cl->size() <= 5.5f ) {
                            if ( cl->stats.num_antecedents_rel <= 0.172496750951f ) {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                    if ( cl->stats.size_rel <= 0.189761340618f ) {
                                        if ( rdb0_last_touched_diff <= 15418.0f ) {
                                            return 54.0/360.2;
                                        } else {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 8500502.0f ) {
                                                if ( cl->stats.rdb1_last_touched_diff <= 16385.0f ) {
                                                    return 72.0/258.1;
                                                } else {
                                                    return 85.0/234.1;
                                                }
                                            } else {
                                                return 47.0/314.2;
                                            }
                                        }
                                    } else {
                                        return 41.0/324.2;
                                    }
                                } else {
                                    return 182.0/454.3;
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 11089.5f ) {
                                    return 63.0/338.2;
                                } else {
                                    return 72.0/582.3;
                                }
                            }
                        } else {
                            if ( cl->stats.num_antecedents_rel <= 0.233201056719f ) {
                                if ( cl->stats.size_rel <= 0.140377566218f ) {
                                    return 104.0/626.4;
                                } else {
                                    if ( cl->stats.size_rel <= 0.198274940252f ) {
                                        return 140.0/282.2;
                                    } else {
                                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 12073.0f ) {
                                                return 84.0/366.2;
                                            } else {
                                                return 58.0/374.2;
                                            }
                                        } else {
                                            if ( cl->stats.sum_uip1_used <= 27.5f ) {
                                                return 130.0/216.1;
                                            } else {
                                                return 73.0/588.3;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 22.5f ) {
                                    return 137.0/246.1;
                                } else {
                                    return 151.0/416.2;
                                }
                            }
                        }
                    } else {
                        if ( cl->size() <= 6.5f ) {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.0539188683033f ) {
                                return 61.0/410.2;
                            } else {
                                if ( cl->stats.size_rel <= 0.166133254766f ) {
                                    return 27.0/374.2;
                                } else {
                                    return 34.0/564.3;
                                }
                            }
                        } else {
                            return 77.0/542.3;
                        }
                    }
                }
            } else {
                if ( cl->stats.sum_uip1_used <= 14.5f ) {
                    if ( cl->stats.sum_delta_confl_uip1_used <= 288.5f ) {
                        if ( cl->size() <= 24.5f ) {
                            if ( cl->stats.glue_rel_long <= 0.56645154953f ) {
                                return 196.0/246.1;
                            } else {
                                return 190.0/124.1;
                            }
                        } else {
                            return 187.0/74.0;
                        }
                    } else {
                        if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->size() <= 18.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.562952518463f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.453376054764f ) {
                                            return 122.0/284.2;
                                        } else {
                                            return 176.0/220.1;
                                        }
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.193785086274f ) {
                                            return 73.0/256.1;
                                        } else {
                                            return 105.0/222.1;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.202141672373f ) {
                                        return 162.0/116.1;
                                    } else {
                                        return 203.0/226.1;
                                    }
                                }
                            } else {
                                if ( cl->size() <= 10.5f ) {
                                    return 121.0/200.1;
                                } else {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 1.10005664825f ) {
                                            return 234.0/206.1;
                                        } else {
                                            return 218.0/104.1;
                                        }
                                    } else {
                                        return 146.0/174.1;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 25590.0f ) {
                                    return 71.0/314.2;
                                } else {
                                    return 121.0/190.1;
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.510878443718f ) {
                                    return 60.0/288.2;
                                } else {
                                    return 77.0/278.2;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_overlap_literals_rel <= 0.110644489527f ) {
                        if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                            if ( cl->stats.sum_uip1_used <= 36.5f ) {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                    if ( cl->stats.dump_number <= 20.5f ) {
                                        if ( rdb0_last_touched_diff <= 24363.5f ) {
                                            return 123.0/368.2;
                                        } else {
                                            return 112.0/252.1;
                                        }
                                    } else {
                                        return 175.0/184.1;
                                    }
                                } else {
                                    return 69.0/450.3;
                                }
                            } else {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 5.5f ) {
                                    if ( cl->size() <= 13.5f ) {
                                        if ( cl->stats.num_overlap_literals <= 6.5f ) {
                                            if ( cl->stats.glue_rel_queue <= 0.449036747217f ) {
                                                return 51.0/340.2;
                                            } else {
                                                return 34.0/434.2;
                                            }
                                        } else {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.204861104488f ) {
                                                return 48.0/310.2;
                                            } else {
                                                return 79.0/324.2;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.sum_uip1_used <= 157.5f ) {
                                            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                                return 135.0/524.3;
                                            } else {
                                                return 94.0/216.1;
                                            }
                                        } else {
                                            return 85.0/516.3;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 178.0f ) {
                                        return 77.0/702.4;
                                    } else {
                                        return 20.0/474.3;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 1403.5f ) {
                                if ( cl->stats.used_for_uip_creation <= 18.5f ) {
                                    if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 8287442.0f ) {
                                            return 24.0/542.3;
                                        } else {
                                            return 9.0/436.2;
                                        }
                                    } else {
                                        return 39.0/504.3;
                                    }
                                } else {
                                    if ( cl->size() <= 10.5f ) {
                                        return 2.0/406.2;
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 351.0f ) {
                                            if ( cl->stats.num_antecedents_rel <= 0.105322822928f ) {
                                                return 12.0/538.3;
                                            } else {
                                                return 22.0/532.3;
                                            }
                                        } else {
                                            if ( cl->stats.size_rel <= 0.275041878223f ) {
                                                return 4.0/442.2;
                                            } else {
                                                return 7.0/418.2;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_uip1_used <= 322.5f ) {
                                    if ( rdb0_last_touched_diff <= 1331.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0214689299464f ) {
                                            return 42.0/410.2;
                                        } else {
                                            if ( cl->stats.size_rel <= 0.358045518398f ) {
                                                return 20.0/458.3;
                                            } else {
                                                return 10.0/408.2;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.size_rel <= 0.30216729641f ) {
                                            return 83.0/630.4;
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 5029.0f ) {
                                                return 24.0/440.2;
                                            } else {
                                                return 43.0/430.2;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 6.5f ) {
                                        return 11.0/380.2;
                                    } else {
                                        return 1.0/476.3;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 24701.5f ) {
                                if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                    if ( cl->stats.size_rel <= 0.676177978516f ) {
                                        if ( cl->stats.sum_uip1_used <= 78.5f ) {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 2341283.5f ) {
                                                return 67.0/388.2;
                                            } else {
                                                return 76.0/242.1;
                                            }
                                        } else {
                                            return 33.0/402.2;
                                        }
                                    } else {
                                        return 121.0/286.2;
                                    }
                                } else {
                                    return 48.0/610.3;
                                }
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.302789628506f ) {
                                        return 116.0/350.2;
                                    } else {
                                        return 127.0/208.1;
                                    }
                                } else {
                                    return 60.0/320.2;
                                }
                            }
                        } else {
                            if ( cl->stats.num_antecedents_rel <= 0.335181117058f ) {
                                if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                    return 84.0/422.2;
                                } else {
                                    if ( cl->stats.dump_number <= 6.5f ) {
                                        return 36.0/454.3;
                                    } else {
                                        return 27.0/642.4;
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_uip1_used <= 83.0f ) {
                                    return 53.0/512.3;
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.65828204155f ) {
                                        return 11.0/578.3;
                                    } else {
                                        return 4.0/548.3;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.sum_delta_confl_uip1_used <= 9126.5f ) {
                if ( cl->stats.num_overlap_literals <= 32.5f ) {
                    if ( rdb0_last_touched_diff <= 55001.5f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 23.5f ) {
                            return 108.0/248.1;
                        } else {
                            if ( rdb0_last_touched_diff <= 29057.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 6050.0f ) {
                                    return 167.0/148.1;
                                } else {
                                    return 171.0/182.1;
                                }
                            } else {
                                return 177.0/90.1;
                            }
                        }
                    } else {
                        return 205.0/86.0;
                    }
                } else {
                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                        if ( cl->stats.num_overlap_literals_rel <= 0.44512668252f ) {
                            return 257.0/172.1;
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 21.0631370544f ) {
                                if ( cl->stats.glue_rel_long <= 0.895485877991f ) {
                                    return 217.0/190.1;
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 204.5f ) {
                                        return 185.0/18.0;
                                    } else {
                                        return 200.0/18.0;
                                    }
                                }
                            } else {
                                return 303.0/36.0;
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 43350.5f ) {
                            if ( cl->size() <= 15.5f ) {
                                return 208.0/86.0;
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 498.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.804823458195f ) {
                                        return 287.0/34.0;
                                    } else {
                                        return 200.0/48.0;
                                    }
                                } else {
                                    return 216.0/8.0;
                                }
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 327.0f ) {
                                if ( cl->stats.glue <= 12.5f ) {
                                    return 201.0/34.0;
                                } else {
                                    return 268.0/22.0;
                                }
                            } else {
                                return 254.0/10.0;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.sum_delta_confl_uip1_used <= 1395350.5f ) {
                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.777318656445f ) {
                        if ( cl->stats.dump_number <= 7.5f ) {
                            if ( cl->size() <= 7.5f ) {
                                return 44.0/518.3;
                            } else {
                                if ( cl->stats.dump_number <= 3.5f ) {
                                    if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                        return 104.0/330.2;
                                    } else {
                                        return 36.0/470.3;
                                    }
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 270139.5f ) {
                                        return 109.0/222.1;
                                    } else {
                                        return 56.0/356.2;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 13.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.158820748329f ) {
                                    return 113.0/172.1;
                                } else {
                                    return 186.0/144.1;
                                }
                            } else {
                                return 86.0/266.2;
                            }
                        }
                    } else {
                        if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                            if ( cl->size() <= 18.5f ) {
                                if ( cl->stats.sum_uip1_used <= 7.5f ) {
                                    if ( cl->stats.dump_number <= 6.5f ) {
                                        return 122.0/200.1;
                                    } else {
                                        return 199.0/134.1;
                                    }
                                } else {
                                    return 138.0/378.2;
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 3906.5f ) {
                                    return 124.0/174.1;
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 12.5f ) {
                                        if ( cl->stats.glue_rel_long <= 1.05932509899f ) {
                                            if ( cl->stats.num_total_lits_antecedents <= 136.0f ) {
                                                return 209.0/136.1;
                                            } else {
                                                return 194.0/46.0;
                                            }
                                        } else {
                                            return 193.0/42.0;
                                        }
                                    } else {
                                        return 144.0/224.1;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 18.5f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.298582911491f ) {
                                    return 85.0/254.1;
                                } else {
                                    return 188.0/308.2;
                                }
                            } else {
                                return 85.0/526.3;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_last_touched_diff <= 7824.0f ) {
                        if ( rdb0_last_touched_diff <= 5062.0f ) {
                            if ( cl->stats.num_antecedents_rel <= 0.154414206743f ) {
                                if ( cl->size() <= 57.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0480868853629f ) {
                                        return 5.0/406.2;
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.1028245911f ) {
                                            return 38.0/404.2;
                                        } else {
                                            return 23.0/446.3;
                                        }
                                    }
                                } else {
                                    return 50.0/360.2;
                                }
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 10154326.0f ) {
                                    if ( cl->stats.used_for_uip_creation <= 8.5f ) {
                                        return 49.0/348.2;
                                    } else {
                                        return 9.0/514.3;
                                    }
                                } else {
                                    if ( cl->size() <= 34.5f ) {
                                        return 8.0/708.4;
                                    } else {
                                        if ( cl->stats.dump_number <= 23.5f ) {
                                            return 4.0/400.2;
                                        } else {
                                            return 21.0/372.2;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                return 99.0/256.1;
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.234784469008f ) {
                                    if ( cl->stats.dump_number <= 23.5f ) {
                                        return 42.0/446.3;
                                    } else {
                                        return 64.0/368.2;
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.316449999809f ) {
                                        return 96.0/268.2;
                                    } else {
                                        return 53.0/352.2;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                            if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                if ( cl->size() <= 13.5f ) {
                                    return 91.0/418.2;
                                } else {
                                    if ( cl->stats.dump_number <= 35.5f ) {
                                        return 77.0/288.2;
                                    } else {
                                        return 107.0/228.1;
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 14031594.0f ) {
                                    return 90.0/544.3;
                                } else {
                                    return 28.0/398.2;
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.899183630943f ) {
                                return 114.0/330.2;
                            } else {
                                return 171.0/268.2;
                            }
                        }
                    }
                }
            }
        }
    } else {
        if ( cl->stats.sum_uip1_used <= 11.5f ) {
            if ( cl->stats.size_rel <= 0.495276510715f ) {
                if ( cl->stats.num_overlap_literals <= 29.5f ) {
                    if ( cl->stats.glue_rel_long <= 0.640203714371f ) {
                        if ( cl->stats.rdb1_act_ranking_top_10 <= 5.5f ) {
                            if ( cl->stats.dump_number <= 6.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0316746048629f ) {
                                    return 165.0/326.2;
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 16.5f ) {
                                        return 63.0/332.2;
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.219590336084f ) {
                                            return 103.0/230.1;
                                        } else {
                                            return 72.0/290.2;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 176768.0f ) {
                                    if ( cl->stats.size_rel <= 0.176740407944f ) {
                                        return 196.0/260.1;
                                    } else {
                                        if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                                            return 176.0/106.1;
                                        } else {
                                            return 250.0/248.1;
                                        }
                                    }
                                } else {
                                    return 179.0/100.1;
                                }
                            }
                        } else {
                            if ( cl->stats.dump_number <= 10.5f ) {
                                if ( cl->stats.num_overlap_literals <= 7.5f ) {
                                    return 100.0/254.1;
                                } else {
                                    return 127.0/188.1;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0495858564973f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 19.5f ) {
                                        return 223.0/74.0;
                                    } else {
                                        return 160.0/76.0;
                                    }
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 4.5f ) {
                                        return 289.0/164.1;
                                    } else {
                                        return 140.0/158.1;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.size_rel <= 0.402323842049f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 5.5f ) {
                                if ( cl->size() <= 11.5f ) {
                                    if ( cl->stats.dump_number <= 11.5f ) {
                                        return 204.0/340.2;
                                    } else {
                                        return 146.0/136.1;
                                    }
                                } else {
                                    return 204.0/108.1;
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 154287.0f ) {
                                    return 220.0/190.1;
                                } else {
                                    return 248.0/98.1;
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 73840.5f ) {
                                return 169.0/158.1;
                            } else {
                                return 292.0/68.0;
                            }
                        }
                    }
                } else {
                    if ( cl->size() <= 7.5f ) {
                        return 224.0/390.2;
                    } else {
                        if ( cl->stats.glue_rel_long <= 0.721937358379f ) {
                            if ( cl->stats.num_overlap_literals <= 173.5f ) {
                                if ( rdb0_last_touched_diff <= 54048.0f ) {
                                    return 161.0/218.1;
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.208285033703f ) {
                                        return 191.0/34.0;
                                    } else {
                                        return 241.0/144.1;
                                    }
                                }
                            } else {
                                return 184.0/66.0;
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.855112493038f ) {
                                return 335.0/170.1;
                            } else {
                                if ( cl->size() <= 10.5f ) {
                                    return 201.0/84.0;
                                } else {
                                    if ( cl->size() <= 56.5f ) {
                                        if ( cl->stats.rdb1_act_ranking_top_10 <= 5.5f ) {
                                            if ( cl->stats.num_total_lits_antecedents <= 124.5f ) {
                                                return 204.0/46.0;
                                            } else {
                                                if ( cl->stats.glue_rel_queue <= 1.02135646343f ) {
                                                    return 248.0/24.0;
                                                } else {
                                                    return 364.0/10.0;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 12.3386478424f ) {
                                                return 244.0/82.0;
                                            } else {
                                                return 197.0/42.0;
                                            }
                                        }
                                    } else {
                                        return 199.0/72.0;
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.num_total_lits_antecedents <= 88.5f ) {
                    if ( cl->size() <= 9.5f ) {
                        return 214.0/282.2;
                    } else {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 8.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 41814.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 22617.0f ) {
                                    return 277.0/68.0;
                                } else {
                                    return 240.0/138.1;
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 9.5f ) {
                                    if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                        if ( rdb0_last_touched_diff <= 71292.0f ) {
                                            return 203.0/50.0;
                                        } else {
                                            if ( cl->stats.num_overlap_literals <= 15.5f ) {
                                                return 335.0/58.0;
                                            } else {
                                                if ( cl->stats.num_overlap_literals_rel <= 0.4230427742f ) {
                                                    if ( cl->stats.glue <= 11.5f ) {
                                                        return 258.0/22.0;
                                                    } else {
                                                        return 222.0/2.0;
                                                    }
                                                } else {
                                                    return 238.0/40.0;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 178803.0f ) {
                                            if ( cl->stats.dump_number <= 11.5f ) {
                                                return 202.0/56.0;
                                            } else {
                                                return 190.0/70.0;
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_act_ranking_top_10 <= 7.5f ) {
                                                return 190.0/64.0;
                                            } else {
                                                return 378.0/48.0;
                                            }
                                        }
                                    }
                                } else {
                                    return 300.0/18.0;
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.677189767361f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.323610365391f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 171980.0f ) {
                                        if ( cl->stats.sum_uip1_used <= 4.5f ) {
                                            return 167.0/106.1;
                                        } else {
                                            return 154.0/172.1;
                                        }
                                    } else {
                                        return 169.0/78.0;
                                    }
                                } else {
                                    return 312.0/130.1;
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 76115.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 15657.0f ) {
                                        return 158.0/116.1;
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.973106980324f ) {
                                            if ( cl->stats.glue_rel_long <= 0.817942082882f ) {
                                                return 164.0/78.0;
                                            } else {
                                                return 162.0/100.1;
                                            }
                                        } else {
                                            return 232.0/88.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 43.5f ) {
                                        return 281.0/46.0;
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 1.01999998093f ) {
                                            if ( cl->stats.dump_number <= 32.5f ) {
                                                if ( cl->stats.rdb1_last_touched_diff <= 138971.5f ) {
                                                    return 174.0/100.1;
                                                } else {
                                                    return 158.0/70.0;
                                                }
                                            } else {
                                                return 247.0/42.0;
                                            }
                                        } else {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.183741956949f ) {
                                                return 295.0/36.0;
                                            } else {
                                                if ( cl->size() <= 19.5f ) {
                                                    return 250.0/80.0;
                                                } else {
                                                    return 215.0/34.0;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( rdb0_last_touched_diff <= 18062.0f ) {
                        if ( cl->stats.num_overlap_literals_rel <= 0.635105848312f ) {
                            return 131.0/128.1;
                        } else {
                            return 232.0/64.0;
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 241.5f ) {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.51265001297f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 138.0f ) {
                                    if ( cl->stats.num_overlap_literals <= 36.5f ) {
                                        return 259.0/58.0;
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.331597268581f ) {
                                            return 266.0/8.0;
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 109357.5f ) {
                                                return 188.0/56.0;
                                            } else {
                                                return 231.0/20.0;
                                            }
                                        }
                                    }
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                        return 283.0/86.0;
                                    } else {
                                        return 177.0/120.1;
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 874.5f ) {
                                    if ( rdb0_last_touched_diff <= 50380.0f ) {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 6.1911110878f ) {
                                            return 191.0/80.0;
                                        } else {
                                            return 349.0/64.0;
                                        }
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.820505142212f ) {
                                            if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                                if ( cl->stats.glue_rel_queue <= 1.02330625057f ) {
                                                    return 193.0/14.0;
                                                } else {
                                                    return 248.0/2.0;
                                                }
                                            } else {
                                                if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                                    return 199.0/28.0;
                                                } else {
                                                    return 216.0/10.0;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 1.96347427368f ) {
                                                return 265.0/80.0;
                                            } else {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.898785710335f ) {
                                                    return 207.0/60.0;
                                                } else {
                                                    if ( cl->stats.antec_num_total_lits_rel <= 1.60712635517f ) {
                                                        if ( cl->stats.rdb1_act_ranking_top_10 <= 6.5f ) {
                                                            if ( cl->size() <= 20.5f ) {
                                                                return 229.0/22.0;
                                                            } else {
                                                                if ( cl->stats.antec_num_total_lits_rel <= 1.15890359879f ) {
                                                                    return 246.0/2.0;
                                                                } else {
                                                                    return 200.0/12.0;
                                                                }
                                                            }
                                                        } else {
                                                            if ( rdb0_last_touched_diff <= 266636.0f ) {
                                                                return 368.0/14.0;
                                                            } else {
                                                                return 207.0/46.0;
                                                            }
                                                        }
                                                    } else {
                                                        return 318.0/68.0;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 16.5f ) {
                                        return 305.0/134.1;
                                    } else {
                                        if ( cl->stats.glue <= 11.5f ) {
                                            return 184.0/54.0;
                                        } else {
                                            return 224.0/34.0;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals <= 415.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 2.35361838341f ) {
                                    if ( rdb0_last_touched_diff <= 43565.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.965744256973f ) {
                                            return 229.0/84.0;
                                        } else {
                                            return 230.0/28.0;
                                        }
                                    } else {
                                        if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                            if ( cl->stats.size_rel <= 0.775999069214f ) {
                                                return 236.0/22.0;
                                            } else {
                                                if ( cl->stats.num_total_lits_antecedents <= 373.5f ) {
                                                    if ( cl->stats.num_overlap_literals_rel <= 1.12592625618f ) {
                                                        return 274.0/8.0;
                                                    } else {
                                                        return 269.0/20.0;
                                                    }
                                                } else {
                                                    return 1;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 0.939314246178f ) {
                                                return 206.0/56.0;
                                            } else {
                                                if ( cl->stats.num_antecedents_rel <= 0.339872658253f ) {
                                                    return 293.0/6.0;
                                                } else {
                                                    if ( cl->stats.size_rel <= 1.41687893867f ) {
                                                        return 327.0/64.0;
                                                    } else {
                                                        return 345.0/28.0;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    return 350.0/94.1;
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.938928723335f ) {
                                    return 291.0/70.0;
                                } else {
                                    if ( cl->stats.dump_number <= 28.5f ) {
                                        if ( cl->stats.glue <= 41.5f ) {
                                            if ( rdb0_last_touched_diff <= 63544.5f ) {
                                                return 285.0/20.0;
                                            } else {
                                                if ( cl->stats.num_overlap_literals_rel <= 3.05964660645f ) {
                                                    return 1;
                                                } else {
                                                    return 267.0/8.0;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.dump_number <= 13.5f ) {
                                                if ( cl->stats.antecedents_glue_long_reds_var <= 169.389648438f ) {
                                                    return 254.0/42.0;
                                                } else {
                                                    return 268.0/10.0;
                                                }
                                            } else {
                                                return 257.0/6.0;
                                            }
                                        }
                                    } else {
                                        return 202.0/26.0;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.rdb1_last_touched_diff <= 63920.0f ) {
                if ( rdb0_last_touched_diff <= 34841.0f ) {
                    if ( cl->stats.sum_uip1_used <= 55.5f ) {
                        if ( cl->stats.dump_number <= 12.5f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                return 55.0/352.2;
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.702676177025f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.102387145162f ) {
                                        return 81.0/296.2;
                                    } else {
                                        return 104.0/582.3;
                                    }
                                } else {
                                    return 113.0/274.2;
                                }
                            }
                        } else {
                            return 185.0/262.1;
                        }
                    } else {
                        if ( cl->stats.num_overlap_literals_rel <= 0.131030023098f ) {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.0622243657708f ) {
                                return 46.0/360.2;
                            } else {
                                return 60.0/692.4;
                            }
                        } else {
                            return 15.0/484.3;
                        }
                    }
                } else {
                    if ( cl->stats.sum_uip1_used <= 60.5f ) {
                        if ( cl->stats.dump_number <= 19.5f ) {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.149425595999f ) {
                                return 131.0/516.3;
                            } else {
                                if ( cl->stats.num_overlap_literals <= 25.5f ) {
                                    return 121.0/294.2;
                                } else {
                                    return 124.0/172.1;
                                }
                            }
                        } else {
                            if ( cl->stats.dump_number <= 33.5f ) {
                                return 136.0/150.1;
                            } else {
                                return 174.0/116.1;
                            }
                        }
                    } else {
                        if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                            return 110.0/502.3;
                        } else {
                            return 65.0/514.3;
                        }
                    }
                }
            } else {
                if ( cl->stats.glue_rel_queue <= 0.715462207794f ) {
                    if ( cl->stats.num_overlap_literals <= 6.5f ) {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 9599224.0f ) {
                            if ( cl->size() <= 7.5f ) {
                                if ( cl->stats.glue_rel_queue <= 0.357250720263f ) {
                                    return 111.0/284.2;
                                } else {
                                    return 108.0/194.1;
                                }
                            } else {
                                if ( cl->stats.sum_uip1_used <= 28.5f ) {
                                    return 135.0/138.1;
                                } else {
                                    return 118.0/200.1;
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.354017913342f ) {
                                return 110.0/214.1;
                            } else {
                                return 98.0/292.2;
                            }
                        }
                    } else {
                        if ( cl->stats.sum_uip1_used <= 20.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 127302.5f ) {
                                return 134.0/136.1;
                            } else {
                                return 300.0/168.1;
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 178728.0f ) {
                                if ( cl->size() <= 11.5f ) {
                                    if ( cl->stats.dump_number <= 25.5f ) {
                                        return 87.0/262.1;
                                    } else {
                                        return 169.0/358.2;
                                    }
                                } else {
                                    return 211.0/362.2;
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 52.5f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0959881469607f ) {
                                        return 158.0/166.1;
                                    } else {
                                        return 99.0/226.1;
                                    }
                                } else {
                                    return 135.0/120.1;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.sum_delta_confl_uip1_used <= 1770253.5f ) {
                        if ( rdb0_act_ranking_top_10 <= 7.5f ) {
                            if ( cl->stats.dump_number <= 23.5f ) {
                                return 196.0/136.1;
                            } else {
                                return 171.0/74.0;
                            }
                        } else {
                            return 192.0/60.0;
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 187396.5f ) {
                            return 188.0/368.2;
                        } else {
                            return 210.0/168.1;
                        }
                    }
                }
            }
        }
    }
}

static double estimator_should_keep_long_conf2_cluster0_5(
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
    if ( cl->stats.sum_uip1_used <= 10.5f ) {
        if ( cl->stats.glue <= 8.5f ) {
            if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                if ( cl->stats.size_rel <= 0.588707327843f ) {
                    if ( rdb0_last_touched_diff <= 12682.0f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 36.5f ) {
                            if ( cl->stats.num_overlap_literals_rel <= 0.0241864770651f ) {
                                return 173.0/456.3;
                            } else {
                                if ( cl->stats.dump_number <= 4.5f ) {
                                    if ( cl->stats.size_rel <= 0.325996518135f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 18.5f ) {
                                            return 34.0/432.2;
                                        } else {
                                            return 52.0/404.2;
                                        }
                                    } else {
                                        return 102.0/406.2;
                                    }
                                } else {
                                    return 122.0/372.2;
                                }
                            }
                        } else {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.327069461346f ) {
                                return 156.0/290.2;
                            } else {
                                return 84.0/316.2;
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 43995.0f ) {
                            if ( cl->size() <= 9.5f ) {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                    if ( cl->stats.size_rel <= 0.140630304813f ) {
                                        return 120.0/530.3;
                                    } else {
                                        if ( cl->stats.dump_number <= 3.5f ) {
                                            if ( cl->stats.num_overlap_literals <= 11.5f ) {
                                                return 79.0/254.1;
                                            } else {
                                                return 94.0/238.1;
                                            }
                                        } else {
                                            return 142.0/258.1;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 16.5f ) {
                                        return 97.0/238.1;
                                    } else {
                                        return 160.0/278.2;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.531043171883f ) {
                                    return 224.0/374.2;
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 36.5f ) {
                                        return 132.0/184.1;
                                    } else {
                                        return 228.0/152.1;
                                    }
                                }
                            }
                        } else {
                            if ( cl->size() <= 7.5f ) {
                                return 165.0/406.2;
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.2301312536f ) {
                                    return 303.0/188.1;
                                } else {
                                    return 160.0/160.1;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.sum_uip1_used <= 3.5f ) {
                        if ( cl->stats.num_antecedents_rel <= 0.242287993431f ) {
                            return 151.0/108.1;
                        } else {
                            if ( cl->stats.num_overlap_literals <= 37.5f ) {
                                return 203.0/112.1;
                            } else {
                                return 227.0/54.0;
                            }
                        }
                    } else {
                        if ( cl->size() <= 15.5f ) {
                            return 135.0/244.1;
                        } else {
                            return 291.0/218.1;
                        }
                    }
                }
            } else {
                if ( cl->stats.size_rel <= 0.400738447905f ) {
                    if ( cl->stats.rdb1_last_touched_diff <= 56855.0f ) {
                        if ( cl->stats.glue_rel_long <= 0.373731583357f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 22.5f ) {
                                return 122.0/424.2;
                            } else {
                                return 120.0/274.2;
                            }
                        } else {
                            if ( cl->size() <= 5.5f ) {
                                return 133.0/240.1;
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.725331902504f ) {
                                    if ( cl->stats.num_overlap_literals <= 15.5f ) {
                                        return 211.0/290.2;
                                    } else {
                                        return 145.0/146.1;
                                    }
                                } else {
                                    return 222.0/194.1;
                                }
                            }
                        }
                    } else {
                        if ( cl->size() <= 8.5f ) {
                            if ( cl->stats.dump_number <= 34.5f ) {
                                if ( cl->stats.size_rel <= 0.306960165501f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0485047772527f ) {
                                        if ( cl->stats.size_rel <= 0.123872712255f ) {
                                            return 154.0/160.1;
                                        } else {
                                            return 187.0/82.0;
                                        }
                                    } else {
                                        if ( cl->size() <= 5.5f ) {
                                            return 137.0/216.1;
                                        } else {
                                            return 260.0/282.2;
                                        }
                                    }
                                } else {
                                    return 100.0/184.1;
                                }
                            } else {
                                return 293.0/116.1;
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.415442109108f ) {
                                return 212.0/178.1;
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 29.5f ) {
                                    return 232.0/132.1;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 125498.0f ) {
                                        return 163.0/80.0;
                                    } else {
                                        return 222.0/52.0;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.dump_number <= 14.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 42660.0f ) {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.543599128723f ) {
                                if ( cl->size() <= 10.5f ) {
                                    return 111.0/194.1;
                                } else {
                                    if ( cl->stats.size_rel <= 0.650153875351f ) {
                                        return 148.0/150.1;
                                    } else {
                                        return 147.0/108.1;
                                    }
                                }
                            } else {
                                return 215.0/92.1;
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 82861.0f ) {
                                if ( cl->size() <= 11.5f ) {
                                    return 134.0/170.1;
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.697269260883f ) {
                                        return 153.0/90.1;
                                    } else {
                                        return 237.0/50.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 100.5f ) {
                                    if ( cl->stats.size_rel <= 0.698506116867f ) {
                                        return 189.0/58.0;
                                    } else {
                                        return 176.0/48.0;
                                    }
                                } else {
                                    return 160.0/104.1;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.sum_uip1_used <= 7.5f ) {
                            if ( cl->stats.num_overlap_literals <= 11.5f ) {
                                if ( cl->stats.glue_rel_long <= 0.671028494835f ) {
                                    return 308.0/76.0;
                                } else {
                                    return 217.0/14.0;
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 227173.0f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.417662322521f ) {
                                            return 172.0/90.1;
                                        } else {
                                            return 235.0/42.0;
                                        }
                                    } else {
                                        if ( cl->stats.dump_number <= 38.5f ) {
                                            return 173.0/106.1;
                                        } else {
                                            return 186.0/70.0;
                                        }
                                    }
                                } else {
                                    return 349.0/58.0;
                                }
                            }
                        } else {
                            return 168.0/120.1;
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.sum_delta_confl_uip1_used <= 169.5f ) {
                if ( cl->stats.glue_rel_queue <= 0.865448653698f ) {
                    if ( cl->stats.num_overlap_literals_rel <= 0.137659445405f ) {
                        if ( cl->stats.num_overlap_literals <= 8.5f ) {
                            return 245.0/182.1;
                        } else {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                                return 173.0/126.1;
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                    return 211.0/62.0;
                                } else {
                                    return 192.0/86.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->size() <= 13.5f ) {
                            return 185.0/90.1;
                        } else {
                            if ( cl->stats.dump_number <= 2.5f ) {
                                return 259.0/122.1;
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 71.5f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 2.24499988556f ) {
                                        return 216.0/14.0;
                                    } else {
                                        return 188.0/32.0;
                                    }
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                        if ( cl->stats.size_rel <= 0.926928639412f ) {
                                            return 246.0/48.0;
                                        } else {
                                            return 258.0/20.0;
                                        }
                                    } else {
                                        if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                            return 291.0/128.1;
                                        } else {
                                            return 241.0/38.0;
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.0331632643938f ) {
                        if ( cl->stats.dump_number <= 2.5f ) {
                            return 144.0/142.1;
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 196060.0f ) {
                                if ( cl->stats.glue_rel_long <= 1.18596577644f ) {
                                    return 252.0/120.1;
                                } else {
                                    return 233.0/56.0;
                                }
                            } else {
                                return 234.0/34.0;
                            }
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 41.5f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 34.5f ) {
                                return 166.0/110.1;
                            } else {
                                return 212.0/66.0;
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 1.07150506973f ) {
                                if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.869635403156f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 48858.5f ) {
                                            return 173.0/82.0;
                                        } else {
                                            return 236.0/36.0;
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 112.5f ) {
                                            if ( cl->stats.glue <= 16.5f ) {
                                                if ( cl->stats.size_rel <= 0.929363131523f ) {
                                                    if ( cl->stats.dump_number <= 5.5f ) {
                                                        return 208.0/48.0;
                                                    } else {
                                                        return 208.0/18.0;
                                                    }
                                                } else {
                                                    if ( cl->stats.glue_rel_long <= 0.999486088753f ) {
                                                        return 268.0/6.0;
                                                    } else {
                                                        return 201.0/18.0;
                                                    }
                                                }
                                            } else {
                                                return 254.0/58.0;
                                            }
                                        } else {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 13.2955150604f ) {
                                                if ( cl->stats.dump_number <= 7.5f ) {
                                                    return 249.0/38.0;
                                                } else {
                                                    return 217.0/4.0;
                                                }
                                            } else {
                                                if ( cl->stats.num_overlap_literals_rel <= 1.20153331757f ) {
                                                    return 344.0/2.0;
                                                } else {
                                                    if ( cl->stats.sum_delta_confl_uip1_used <= 3.5f ) {
                                                        return 300.0/20.0;
                                                    } else {
                                                        return 263.0/6.0;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 7.5f ) {
                                        return 280.0/156.1;
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.861671328545f ) {
                                            if ( cl->stats.num_antecedents_rel <= 0.376200199127f ) {
                                                return 215.0/10.0;
                                            } else {
                                                return 237.0/32.0;
                                            }
                                        } else {
                                            return 418.0/76.0;
                                        }
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 544454.5f ) {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 8.5f ) {
                                        if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                            if ( cl->stats.dump_number <= 8.5f ) {
                                                if ( cl->stats.num_overlap_literals_rel <= 0.1668638587f ) {
                                                    return 189.0/34.0;
                                                } else {
                                                    if ( cl->stats.glue <= 11.5f ) {
                                                        return 308.0/62.0;
                                                    } else {
                                                        if ( rdb0_last_touched_diff <= 21427.0f ) {
                                                            if ( cl->stats.num_total_lits_antecedents <= 414.5f ) {
                                                                return 291.0/42.0;
                                                            } else {
                                                                return 228.0/14.0;
                                                            }
                                                        } else {
                                                            if ( cl->stats.antecedents_glue_long_reds_var <= 20.3611106873f ) {
                                                                if ( cl->stats.num_overlap_literals <= 306.0f ) {
                                                                    if ( cl->stats.antec_num_total_lits_rel <= 0.823457837105f ) {
                                                                        return 253.0/10.0;
                                                                    } else {
                                                                        return 315.0/50.0;
                                                                    }
                                                                } else {
                                                                    return 213.0/4.0;
                                                                }
                                                            } else {
                                                                if ( cl->stats.sum_delta_confl_uip1_used <= 4.5f ) {
                                                                    if ( cl->stats.rdb1_last_touched_diff <= 26087.5f ) {
                                                                        return 206.0/6.0;
                                                                    } else {
                                                                        if ( cl->stats.num_total_lits_antecedents <= 298.5f ) {
                                                                            return 205.0/2.0;
                                                                        } else {
                                                                            return 1;
                                                                        }
                                                                    }
                                                                } else {
                                                                    return 262.0/12.0;
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.glue_rel_queue <= 1.16252040863f ) {
                                                    if ( cl->stats.num_overlap_literals <= 158.0f ) {
                                                        return 240.0/6.0;
                                                    } else {
                                                        return 213.0/22.0;
                                                    }
                                                } else {
                                                    if ( cl->stats.num_overlap_literals_rel <= 1.29642295837f ) {
                                                        if ( cl->stats.num_overlap_literals <= 91.5f ) {
                                                            return 276.0/6.0;
                                                        } else {
                                                            return 1;
                                                        }
                                                    } else {
                                                        return 1;
                                                    }
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.size_rel <= 1.80141282082f ) {
                                                if ( cl->stats.num_overlap_literals <= 61.5f ) {
                                                    if ( cl->stats.num_total_lits_antecedents <= 76.5f ) {
                                                        return 226.0/36.0;
                                                    } else {
                                                        return 231.0/70.0;
                                                    }
                                                } else {
                                                    if ( cl->stats.antecedents_glue_long_reds_var <= 119.951446533f ) {
                                                        if ( cl->stats.antecedents_glue_long_reds_var <= 16.6605911255f ) {
                                                            if ( cl->stats.dump_number <= 12.5f ) {
                                                                return 261.0/80.0;
                                                            } else {
                                                                return 187.0/28.0;
                                                            }
                                                        } else {
                                                            if ( cl->size() <= 44.5f ) {
                                                                return 336.0/24.0;
                                                            } else {
                                                                return 1;
                                                            }
                                                        }
                                                    } else {
                                                        return 183.0/62.0;
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.glue_rel_queue <= 1.48949861526f ) {
                                                    return 303.0/16.0;
                                                } else {
                                                    return 225.0/6.0;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->size() <= 28.5f ) {
                                            return 291.0/26.0;
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 122208.5f ) {
                                                return 254.0/14.0;
                                            } else {
                                                if ( cl->stats.num_overlap_literals <= 244.0f ) {
                                                    return 1;
                                                } else {
                                                    return 202.0/6.0;
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    return 243.0/66.0;
                                }
                            }
                        }
                    }
                }
            } else {
                if ( rdb0_last_touched_diff <= 46769.0f ) {
                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.130469813943f ) {
                        if ( cl->stats.num_overlap_literals_rel <= 0.023337520659f ) {
                            return 120.0/180.1;
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.684930801392f ) {
                                return 71.0/314.2;
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.226852983236f ) {
                                    return 108.0/184.1;
                                } else {
                                    return 114.0/182.1;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue <= 11.5f ) {
                            if ( rdb0_last_touched_diff <= 22450.0f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 56.5f ) {
                                    return 171.0/296.2;
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.541299760342f ) {
                                        return 170.0/104.1;
                                    } else {
                                        return 120.0/190.1;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.843724012375f ) {
                                    return 147.0/116.1;
                                } else {
                                    return 172.0/72.0;
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 4993.0f ) {
                                return 152.0/198.1;
                            } else {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 354.5f ) {
                                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                            return 220.0/86.0;
                                        } else {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.46277731657f ) {
                                                return 207.0/142.1;
                                            } else {
                                                return 200.0/70.0;
                                            }
                                        }
                                    } else {
                                        return 208.0/44.0;
                                    }
                                } else {
                                    return 233.0/146.1;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_overlap_literals <= 27.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 143822.0f ) {
                            if ( cl->stats.glue_rel_queue <= 0.697287797928f ) {
                                return 227.0/248.1;
                            } else {
                                if ( cl->stats.num_overlap_literals <= 14.5f ) {
                                    return 317.0/110.1;
                                } else {
                                    return 187.0/124.1;
                                }
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 1.84375f ) {
                                if ( rdb0_last_touched_diff <= 242728.5f ) {
                                    return 164.0/88.0;
                                } else {
                                    return 236.0/38.0;
                                }
                            } else {
                                return 258.0/28.0;
                            }
                        }
                    } else {
                        if ( cl->stats.glue <= 9.5f ) {
                            return 218.0/88.0;
                        } else {
                            if ( cl->size() <= 119.5f ) {
                                if ( rdb0_last_touched_diff <= 68116.0f ) {
                                    return 323.0/138.1;
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.444907367229f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.824899554253f ) {
                                            return 179.0/82.0;
                                        } else {
                                            return 238.0/60.0;
                                        }
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.612657427788f ) {
                                            if ( cl->stats.num_total_lits_antecedents <= 104.5f ) {
                                                return 198.0/28.0;
                                            } else {
                                                return 264.0/24.0;
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 1.20101046562f ) {
                                                if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                                    return 257.0/44.0;
                                                } else {
                                                    return 239.0/78.0;
                                                }
                                            } else {
                                                return 176.0/78.0;
                                            }
                                        }
                                    }
                                }
                            } else {
                                return 274.0/32.0;
                            }
                        }
                    }
                }
            }
        }
    } else {
        if ( rdb0_last_touched_diff <= 37497.0f ) {
            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                    if ( cl->stats.sum_uip1_used <= 58.5f ) {
                        if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                            if ( rdb0_last_touched_diff <= 9476.0f ) {
                                if ( cl->stats.glue <= 5.5f ) {
                                    return 56.0/418.2;
                                } else {
                                    return 118.0/334.2;
                                }
                            } else {
                                if ( cl->stats.dump_number <= 32.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 21.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.459922939539f ) {
                                            return 106.0/380.2;
                                        } else {
                                            return 47.0/346.2;
                                        }
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.335699856281f ) {
                                            if ( cl->stats.dump_number <= 13.5f ) {
                                                return 137.0/404.2;
                                            } else {
                                                return 110.0/190.1;
                                            }
                                        } else {
                                            return 71.0/344.2;
                                        }
                                    }
                                } else {
                                    return 236.0/296.2;
                                }
                            }
                        } else {
                            if ( cl->stats.size_rel <= 0.639092028141f ) {
                                if ( cl->stats.dump_number <= 14.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 17.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0700202435255f ) {
                                            if ( cl->stats.sum_uip1_used <= 23.5f ) {
                                                return 53.0/290.2;
                                            } else {
                                                return 43.0/400.2;
                                            }
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 13.5f ) {
                                                return 15.0/452.3;
                                            } else {
                                                return 25.0/388.2;
                                            }
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 1363.5f ) {
                                            if ( cl->stats.num_antecedents_rel <= 0.172058984637f ) {
                                                return 57.0/444.3;
                                            } else {
                                                return 19.0/424.2;
                                            }
                                        } else {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.980000019073f ) {
                                                if ( cl->stats.size_rel <= 0.345713913441f ) {
                                                    return 86.0/454.3;
                                                } else {
                                                    return 43.0/328.2;
                                                }
                                            } else {
                                                return 89.0/318.2;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                        if ( cl->stats.sum_uip1_used <= 29.5f ) {
                                            return 181.0/300.2;
                                        } else {
                                            return 102.0/354.2;
                                        }
                                    } else {
                                        if ( cl->stats.dump_number <= 22.5f ) {
                                            return 47.0/330.2;
                                        } else {
                                            return 82.0/358.2;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 14411.5f ) {
                                    return 93.0/438.2;
                                } else {
                                    return 188.0/434.2;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 134524224.0f ) {
                            if ( cl->stats.dump_number <= 66.5f ) {
                                if ( cl->stats.glue_rel_long <= 0.949559569359f ) {
                                    if ( cl->stats.size_rel <= 0.737293601036f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.0592334270477f ) {
                                            if ( cl->stats.used_for_uip_creation <= 11.5f ) {
                                                if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                                                    return 76.0/368.2;
                                                } else {
                                                    return 62.0/418.2;
                                                }
                                            } else {
                                                return 27.0/366.2;
                                            }
                                        } else {
                                            if ( cl->stats.sum_uip1_used <= 172.5f ) {
                                                if ( cl->stats.num_overlap_literals_rel <= 0.136976331472f ) {
                                                    if ( cl->stats.dump_number <= 23.5f ) {
                                                        if ( cl->stats.size_rel <= 0.329135835171f ) {
                                                            if ( cl->stats.glue <= 4.5f ) {
                                                                return 38.0/612.3;
                                                            } else {
                                                                return 20.0/686.4;
                                                            }
                                                        } else {
                                                            return 49.0/520.3;
                                                        }
                                                    } else {
                                                        if ( cl->stats.num_antecedents_rel <= 0.170221045613f ) {
                                                            if ( rdb0_last_touched_diff <= 4085.5f ) {
                                                                return 19.0/388.2;
                                                            } else {
                                                                return 49.0/418.2;
                                                            }
                                                        } else {
                                                            return 50.0/320.2;
                                                        }
                                                    }
                                                } else {
                                                    if ( cl->stats.sum_uip1_used <= 90.5f ) {
                                                        return 31.0/346.2;
                                                    } else {
                                                        return 52.0/328.2;
                                                    }
                                                }
                                            } else {
                                                if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                                    return 52.0/618.3;
                                                } else {
                                                    if ( cl->stats.num_total_lits_antecedents <= 36.5f ) {
                                                        if ( rdb0_last_touched_diff <= 2145.5f ) {
                                                            return 11.0/724.4;
                                                        } else {
                                                            return 36.0/592.3;
                                                        }
                                                    } else {
                                                        return 8.0/434.2;
                                                    }
                                                }
                                            }
                                        }
                                    } else {
                                        return 74.0/434.2;
                                    }
                                } else {
                                    return 93.0/506.3;
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.102045789361f ) {
                                    return 69.0/406.2;
                                } else {
                                    if ( rdb0_last_touched_diff <= 5914.5f ) {
                                        return 73.0/340.2;
                                    } else {
                                        return 100.0/200.1;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 17.5f ) {
                                    return 17.0/494.3;
                                } else {
                                    return 42.0/460.3;
                                }
                            } else {
                                if ( cl->stats.size_rel <= 0.246137320995f ) {
                                    return 3.0/426.2;
                                } else {
                                    return 11.0/402.2;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                        if ( cl->stats.glue <= 7.5f ) {
                            if ( cl->stats.num_overlap_literals <= 21.5f ) {
                                if ( cl->size() <= 7.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.217458635569f ) {
                                        return 55.0/350.2;
                                    } else {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 12.5f ) {
                                            if ( cl->stats.dump_number <= 18.5f ) {
                                                return 30.0/688.4;
                                            } else {
                                                if ( rdb0_last_touched_diff <= 9011.5f ) {
                                                    return 19.0/354.2;
                                                } else {
                                                    return 50.0/376.2;
                                                }
                                            }
                                        } else {
                                            return 25.0/756.4;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.465944170952f ) {
                                        return 84.0/556.3;
                                    } else {
                                        return 52.0/618.3;
                                    }
                                }
                            } else {
                                return 92.0/596.3;
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 119.5f ) {
                                if ( cl->stats.sum_uip1_used <= 38.5f ) {
                                    return 93.0/426.2;
                                } else {
                                    if ( rdb0_last_touched_diff <= 10065.0f ) {
                                        return 15.0/380.2;
                                    } else {
                                        return 43.0/392.2;
                                    }
                                }
                            } else {
                                return 88.0/252.1;
                            }
                        }
                    } else {
                        if ( cl->stats.glue <= 4.5f ) {
                            if ( rdb0_last_touched_diff <= 4792.0f ) {
                                if ( cl->stats.used_for_uip_creation <= 5.5f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 6180182.5f ) {
                                        return 29.0/428.2;
                                    } else {
                                        return 16.0/712.4;
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 64.5f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.0349161848426f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.00463461829349f ) {
                                                return 9.0/828.5;
                                            } else {
                                                if ( cl->stats.sum_delta_confl_uip1_used <= 6861186.5f ) {
                                                    return 24.0/356.2;
                                                } else {
                                                    return 25.0/708.4;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 0.514771223068f ) {
                                                if ( cl->stats.num_overlap_literals_rel <= 0.0683973282576f ) {
                                                    if ( cl->stats.rdb1_last_touched_diff <= 177.0f ) {
                                                        if ( cl->stats.num_total_lits_antecedents <= 11.5f ) {
                                                            return 14.0/386.2;
                                                        } else {
                                                            return 7.0/468.3;
                                                        }
                                                    } else {
                                                        if ( cl->stats.rdb1_used_for_uip_creation <= 8.5f ) {
                                                            return 14.0/690.4;
                                                        } else {
                                                            if ( cl->stats.used_for_uip_creation <= 19.5f ) {
                                                                if ( cl->stats.rdb1_last_touched_diff <= 1417.5f ) {
                                                                    return 0.0/548.3;
                                                                } else {
                                                                    return 3.0/514.3;
                                                                }
                                                            } else {
                                                                if ( cl->stats.sum_delta_confl_uip1_used <= 10545952.0f ) {
                                                                    return 9.0/538.3;
                                                                } else {
                                                                    return 1.0/796.4;
                                                                }
                                                            }
                                                        }
                                                    }
                                                } else {
                                                    if ( cl->stats.sum_uip1_used <= 112.5f ) {
                                                        return 20.0/556.3;
                                                    } else {
                                                        if ( cl->stats.rdb1_used_for_uip_creation <= 24.5f ) {
                                                            return 4.0/490.3;
                                                        } else {
                                                            return 14.0/472.3;
                                                        }
                                                    }
                                                }
                                            } else {
                                                return 31.0/706.4;
                                            }
                                        }
                                    } else {
                                        return 2.0/704.4;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.284902602434f ) {
                                    return 29.0/398.2;
                                } else {
                                    return 27.0/656.4;
                                }
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 113.5f ) {
                                if ( rdb0_last_touched_diff <= 812.0f ) {
                                    if ( cl->stats.used_for_uip_creation <= 11.5f ) {
                                        return 51.0/700.4;
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 35.5f ) {
                                            return 6.0/618.3;
                                        } else {
                                            return 26.0/592.3;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 21.5f ) {
                                        return 57.0/370.2;
                                    } else {
                                        if ( cl->stats.dump_number <= 18.5f ) {
                                            if ( cl->stats.sum_uip1_used <= 75.5f ) {
                                                if ( cl->size() <= 11.5f ) {
                                                    if ( cl->stats.antec_num_total_lits_rel <= 0.117771729827f ) {
                                                        return 24.0/448.3;
                                                    } else {
                                                        return 11.0/420.2;
                                                    }
                                                } else {
                                                    if ( cl->stats.num_antecedents_rel <= 0.213605821133f ) {
                                                        return 54.0/486.3;
                                                    } else {
                                                        return 19.0/408.2;
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.used_for_uip_creation <= 13.5f ) {
                                                    return 15.0/400.2;
                                                } else {
                                                    return 11.0/384.2;
                                                }
                                            }
                                        } else {
                                            return 69.0/384.2;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.267390966415f ) {
                                    if ( cl->size() <= 12.5f ) {
                                        return 35.0/466.3;
                                    } else {
                                        return 15.0/460.3;
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.0594348348677f ) {
                                        if ( cl->stats.used_for_uip_creation <= 36.5f ) {
                                            if ( cl->stats.glue_rel_long <= 0.485697090626f ) {
                                                return 14.0/406.2;
                                            } else {
                                                return 30.0/392.2;
                                            }
                                        } else {
                                            return 12.0/518.3;
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.405109614134f ) {
                                            if ( cl->stats.sum_uip1_used <= 245.5f ) {
                                                if ( cl->stats.glue_rel_queue <= 0.505541205406f ) {
                                                    if ( cl->stats.size_rel <= 0.267183482647f ) {
                                                        return 12.0/414.2;
                                                    } else {
                                                        return 10.0/580.3;
                                                    }
                                                } else {
                                                    if ( cl->stats.rdb1_last_touched_diff <= 1645.0f ) {
                                                        return 32.0/724.4;
                                                    } else {
                                                        return 11.0/572.3;
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.sum_delta_confl_uip1_used <= 226671520.0f ) {
                                                    if ( rdb0_last_touched_diff <= 3698.5f ) {
                                                        if ( cl->stats.num_total_lits_antecedents <= 35.5f ) {
                                                            if ( cl->stats.glue_rel_queue <= 0.519218623638f ) {
                                                                return 8.0/704.4;
                                                            } else {
                                                                return 2.0/464.3;
                                                            }
                                                        } else {
                                                            if ( cl->stats.num_overlap_literals <= 17.5f ) {
                                                                return 0.0/600.3;
                                                            } else {
                                                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.17793366313f ) {
                                                                    return 2.0/488.3;
                                                                } else {
                                                                    return 4.0/428.2;
                                                                }
                                                            }
                                                        }
                                                    } else {
                                                        return 8.0/470.3;
                                                    }
                                                } else {
                                                    if ( rdb0_last_touched_diff <= 867.5f ) {
                                                        return 4.0/622.4;
                                                    } else {
                                                        return 18.0/390.2;
                                                    }
                                                }
                                            }
                                        } else {
                                            return 19.0/554.3;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.sum_uip1_used <= 56.5f ) {
                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.653333306313f ) {
                        if ( cl->stats.dump_number <= 11.5f ) {
                            if ( cl->stats.glue_rel_long <= 0.68950843811f ) {
                                if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.125641554594f ) {
                                            return 59.0/570.3;
                                        } else {
                                            return 57.0/296.2;
                                        }
                                    } else {
                                        if ( cl->stats.size_rel <= 0.154350578785f ) {
                                            return 83.0/274.2;
                                        } else {
                                            if ( cl->stats.sum_uip1_used <= 22.5f ) {
                                                return 52.0/336.2;
                                            } else {
                                                return 55.0/292.2;
                                            }
                                        }
                                    }
                                } else {
                                    return 46.0/648.4;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 14.5f ) {
                                    return 80.0/346.2;
                                } else {
                                    return 110.0/222.1;
                                }
                            }
                        } else {
                            if ( cl->size() <= 11.5f ) {
                                if ( cl->stats.num_overlap_literals <= 5.5f ) {
                                    return 159.0/280.2;
                                } else {
                                    return 142.0/416.2;
                                }
                            } else {
                                return 188.0/264.1;
                            }
                        }
                    } else {
                        if ( cl->size() <= 21.5f ) {
                            if ( rdb0_last_touched_diff <= 18750.0f ) {
                                return 150.0/480.3;
                            } else {
                                return 152.0/314.2;
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 8468.0f ) {
                                return 147.0/248.1;
                            } else {
                                return 190.0/154.1;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_last_touched_diff <= 3073.5f ) {
                        if ( rdb0_last_touched_diff <= 9130.0f ) {
                            if ( cl->stats.size_rel <= 0.145254611969f ) {
                                return 8.0/544.3;
                            } else {
                                return 29.0/638.4;
                            }
                        } else {
                            return 53.0/628.4;
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 11879.5f ) {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.0774592459202f ) {
                                return 89.0/600.3;
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.280938982964f ) {
                                    if ( cl->stats.sum_uip1_used <= 119.5f ) {
                                        return 39.0/400.2;
                                    } else {
                                        return 15.0/420.2;
                                    }
                                } else {
                                    return 48.0/312.2;
                                }
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 137.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0610518679023f ) {
                                    return 123.0/348.2;
                                } else {
                                    return 72.0/406.2;
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 28018.5f ) {
                                    return 53.0/450.3;
                                } else {
                                    return 25.0/542.3;
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                if ( cl->stats.glue <= 10.5f ) {
                    if ( cl->stats.num_total_lits_antecedents <= 27.5f ) {
                        if ( cl->stats.sum_uip1_used <= 76.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 76735.5f ) {
                                if ( cl->stats.dump_number <= 31.5f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 386650.5f ) {
                                        return 65.0/296.2;
                                    } else {
                                        if ( rdb0_last_touched_diff <= 53187.0f ) {
                                            return 124.0/288.2;
                                        } else {
                                            return 82.0/282.2;
                                        }
                                    }
                                } else {
                                    return 216.0/272.2;
                                }
                            } else {
                                return 133.0/186.1;
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 160.5f ) {
                                return 141.0/536.3;
                            } else {
                                if ( cl->stats.size_rel <= 0.232300281525f ) {
                                    return 82.0/434.2;
                                } else {
                                    return 39.0/444.3;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.size_rel <= 0.666194915771f ) {
                            if ( cl->stats.glue_rel_queue <= 0.352267801762f ) {
                                return 150.0/212.1;
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 10682476.0f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 408329.5f ) {
                                        return 85.0/218.1;
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 48614.0f ) {
                                            return 114.0/234.1;
                                        } else {
                                            return 133.0/182.1;
                                        }
                                    }
                                } else {
                                    return 77.0/362.2;
                                }
                            }
                        } else {
                            return 202.0/298.2;
                        }
                    }
                } else {
                    if ( cl->size() <= 58.5f ) {
                        if ( cl->stats.dump_number <= 24.5f ) {
                            return 188.0/410.2;
                        } else {
                            return 186.0/226.1;
                        }
                    } else {
                        return 163.0/136.1;
                    }
                }
            } else {
                if ( cl->stats.sum_delta_confl_uip1_used <= 7858431.5f ) {
                    if ( cl->stats.dump_number <= 15.5f ) {
                        if ( cl->stats.dump_number <= 12.5f ) {
                            if ( cl->stats.sum_uip1_used <= 24.5f ) {
                                return 201.0/356.2;
                            } else {
                                return 109.0/492.3;
                            }
                        } else {
                            return 175.0/288.2;
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 19.5f ) {
                            if ( cl->stats.glue_rel_queue <= 0.480384588242f ) {
                                if ( cl->stats.glue_rel_queue <= 0.267380535603f ) {
                                    return 119.0/172.1;
                                } else {
                                    return 147.0/302.2;
                                }
                            } else {
                                return 173.0/210.1;
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 39.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 49.5f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 1474151.0f ) {
                                        if ( cl->size() <= 13.5f ) {
                                            return 196.0/240.1;
                                        } else {
                                            return 181.0/102.1;
                                        }
                                    } else {
                                        return 249.0/140.1;
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 1.02429759502f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.285874992609f ) {
                                            return 182.0/62.0;
                                        } else {
                                            return 254.0/158.1;
                                        }
                                    } else {
                                        return 197.0/42.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 6.5f ) {
                                    return 155.0/250.1;
                                } else {
                                    return 137.0/280.2;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_last_touched_diff <= 156644.5f ) {
                        if ( rdb0_last_touched_diff <= 95999.0f ) {
                            return 57.0/342.2;
                        } else {
                            return 120.0/368.2;
                        }
                    } else {
                        if ( cl->stats.glue_rel_long <= 0.392495155334f ) {
                            return 146.0/168.1;
                        } else {
                            if ( cl->stats.dump_number <= 87.5f ) {
                                return 116.0/274.2;
                            } else {
                                return 141.0/204.1;
                            }
                        }
                    }
                }
            }
        }
    }
}

static double estimator_should_keep_long_conf2_cluster0_6(
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
    if ( rdb0_last_touched_diff <= 20034.0f ) {
        if ( cl->stats.used_for_uip_creation <= 2.5f ) {
            if ( cl->stats.sum_uip1_used <= 17.5f ) {
                if ( cl->stats.glue_rel_long <= 0.859192609787f ) {
                    if ( cl->size() <= 11.5f ) {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 606470.0f ) {
                            if ( cl->stats.glue_rel_long <= 0.719676971436f ) {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                    if ( rdb0_last_touched_diff <= 16488.0f ) {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 177061.5f ) {
                                            if ( cl->stats.sum_uip1_used <= 3.5f ) {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.0946404784918f ) {
                                                    return 87.0/246.1;
                                                } else {
                                                    return 104.0/378.2;
                                                }
                                            } else {
                                                if ( cl->size() <= 6.5f ) {
                                                    return 59.0/452.3;
                                                } else {
                                                    return 117.0/466.3;
                                                }
                                            }
                                        } else {
                                            return 65.0/476.3;
                                        }
                                    } else {
                                        return 99.0/276.2;
                                    }
                                } else {
                                    if ( cl->size() <= 8.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.377836167812f ) {
                                            return 68.0/320.2;
                                        } else {
                                            return 136.0/344.2;
                                        }
                                    } else {
                                        return 104.0/190.1;
                                    }
                                }
                            } else {
                                return 198.0/410.2;
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 10099.0f ) {
                                return 139.0/162.1;
                            } else {
                                return 120.0/196.1;
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_queue <= 0.642719507217f ) {
                            if ( cl->stats.size_rel <= 0.711994886398f ) {
                                if ( cl->stats.sum_uip1_used <= 5.5f ) {
                                    if ( cl->stats.size_rel <= 0.355496406555f ) {
                                        return 118.0/222.1;
                                    } else {
                                        return 200.0/194.1;
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 12015.5f ) {
                                        return 180.0/336.2;
                                    } else {
                                        return 83.0/260.1;
                                    }
                                }
                            } else {
                                return 220.0/184.1;
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.785694718361f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 272.0f ) {
                                    return 185.0/86.0;
                                } else {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 2.00826454163f ) {
                                            return 100.0/210.1;
                                        } else {
                                            return 159.0/124.1;
                                        }
                                    } else {
                                        return 175.0/86.0;
                                    }
                                }
                            } else {
                                return 244.0/288.2;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.antec_num_total_lits_rel <= 0.393176645041f ) {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 37.5f ) {
                            return 241.0/120.1;
                        } else {
                            if ( cl->size() <= 10.5f ) {
                                return 97.0/250.1;
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                    return 176.0/260.1;
                                } else {
                                    return 191.0/200.1;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.sum_uip1_used <= 2.5f ) {
                            if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                                if ( cl->stats.num_overlap_literals <= 256.0f ) {
                                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                        return 187.0/44.0;
                                    } else {
                                        return 225.0/20.0;
                                    }
                                } else {
                                    return 197.0/2.0;
                                }
                            } else {
                                if ( cl->stats.size_rel <= 1.04667711258f ) {
                                    return 177.0/62.0;
                                } else {
                                    return 197.0/40.0;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 3932.5f ) {
                                return 152.0/172.1;
                            } else {
                                return 317.0/190.1;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.num_total_lits_antecedents <= 27.5f ) {
                    if ( cl->size() <= 5.5f ) {
                        if ( cl->stats.size_rel <= 0.194906428456f ) {
                            if ( cl->stats.sum_uip1_used <= 115.5f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.0321929305792f ) {
                                    return 57.0/520.3;
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0301167294383f ) {
                                        return 113.0/342.2;
                                    } else {
                                        return 89.0/650.4;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                    return 50.0/484.3;
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.268893092871f ) {
                                        return 18.0/344.2;
                                    } else {
                                        return 13.0/544.3;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.dump_number <= 17.5f ) {
                                return 14.0/452.3;
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.108641907573f ) {
                                    return 27.0/358.2;
                                } else {
                                    return 24.0/398.2;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 4.5f ) {
                            if ( cl->stats.num_antecedents_rel <= 0.0495409965515f ) {
                                return 84.0/290.2;
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.129176363349f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0869170874357f ) {
                                        if ( cl->stats.size_rel <= 0.238143771887f ) {
                                            return 48.0/404.2;
                                        } else {
                                            return 26.0/382.2;
                                        }
                                    } else {
                                        return 96.0/580.3;
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 6642.5f ) {
                                        if ( cl->stats.num_overlap_literals <= 6.5f ) {
                                            return 32.0/372.2;
                                        } else {
                                            return 55.0/360.2;
                                        }
                                    } else {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 5342122.0f ) {
                                            return 73.0/464.3;
                                        } else {
                                            return 104.0/368.2;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals_rel <= 0.0632689893246f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0442278906703f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 15.5f ) {
                                        return 20.0/496.3;
                                    } else {
                                        return 58.0/470.3;
                                    }
                                } else {
                                    return 52.0/344.2;
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.522830367088f ) {
                                    return 22.0/418.2;
                                } else {
                                    return 26.0/382.2;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                        if ( cl->stats.sum_uip1_used <= 44.5f ) {
                            if ( cl->stats.glue_rel_queue <= 0.783552289009f ) {
                                if ( cl->stats.glue_rel_long <= 0.538999080658f ) {
                                    return 141.0/242.1;
                                } else {
                                    return 114.0/298.2;
                                }
                            } else {
                                return 165.0/232.1;
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 134.0f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.259278446436f ) {
                                    return 153.0/400.2;
                                } else {
                                    return 68.0/388.2;
                                }
                            } else {
                                return 83.0/650.4;
                            }
                        }
                    } else {
                        if ( cl->stats.num_overlap_literals <= 117.5f ) {
                            if ( cl->stats.dump_number <= 4.5f ) {
                                if ( cl->stats.glue_rel_queue <= 0.604571342468f ) {
                                    return 58.0/312.2;
                                } else {
                                    return 71.0/270.2;
                                }
                            } else {
                                if ( cl->stats.size_rel <= 0.621736884117f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 7.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.106042519212f ) {
                                            return 80.0/362.2;
                                        } else {
                                            return 54.0/526.3;
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 9593.0f ) {
                                            return 12.0/396.2;
                                        } else {
                                            return 55.0/632.4;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 3226.5f ) {
                                        return 41.0/370.2;
                                    } else {
                                        return 65.0/334.2;
                                    }
                                }
                            }
                        } else {
                            return 78.0/280.2;
                        }
                    }
                }
            }
        } else {
            if ( cl->size() <= 6.5f ) {
                if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                    if ( cl->stats.num_overlap_literals_rel <= 0.00862161442637f ) {
                        return 61.0/304.2;
                    } else {
                        if ( cl->stats.sum_uip1_used <= 105.5f ) {
                            if ( cl->stats.used_for_uip_creation <= 6.5f ) {
                                if ( cl->stats.glue_rel_queue <= 0.307850271463f ) {
                                    return 60.0/310.2;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 13015.5f ) {
                                        return 48.0/544.3;
                                    } else {
                                        return 55.0/374.2;
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 1344184.0f ) {
                                    return 24.0/648.4;
                                } else {
                                    return 59.0/594.3;
                                }
                            }
                        } else {
                            if ( cl->stats.size_rel <= 0.257144510746f ) {
                                if ( rdb0_last_touched_diff <= 1714.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.065125413239f ) {
                                        return 6.0/468.3;
                                    } else {
                                        return 8.0/402.2;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.339632302523f ) {
                                        return 20.0/344.2;
                                    } else {
                                        return 12.0/416.2;
                                    }
                                }
                            } else {
                                return 24.0/498.3;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.glue_rel_queue <= 0.483071327209f ) {
                        if ( rdb0_last_touched_diff <= 1045.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 1324.0f ) {
                                if ( cl->stats.sum_uip1_used <= 270.5f ) {
                                    if ( cl->stats.used_for_uip_creation <= 14.5f ) {
                                        return 11.0/400.2;
                                    } else {
                                        return 5.0/718.4;
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.0425644032657f ) {
                                        return 6.0/554.3;
                                    } else {
                                        if ( cl->stats.sum_uip1_used <= 583.0f ) {
                                            return 1.0/470.3;
                                        } else {
                                            return 0.0/674.4;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->size() <= 3.5f ) {
                                    return 3.0/472.3;
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 13.5f ) {
                                        return 14.0/712.4;
                                    } else {
                                        return 17.0/364.2;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 416253.0f ) {
                                return 25.0/428.2;
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.046855751425f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.295130431652f ) {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 18.5f ) {
                                            return 42.0/560.3;
                                        } else {
                                            return 20.0/678.4;
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.019251961261f ) {
                                            return 5.0/450.3;
                                        } else {
                                            return 23.0/708.4;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 4369533.0f ) {
                                        return 2.0/432.2;
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.133961081505f ) {
                                            return 7.0/658.4;
                                        } else {
                                            return 13.0/360.2;
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.0276816617697f ) {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 7791736.0f ) {
                                if ( rdb0_last_touched_diff <= 1907.5f ) {
                                    return 31.0/558.3;
                                } else {
                                    return 37.0/334.2;
                                }
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.125494688749f ) {
                                    return 8.0/766.4;
                                } else {
                                    return 11.0/356.2;
                                }
                            }
                        } else {
                            return 9.0/560.3;
                        }
                    }
                }
            } else {
                if ( cl->stats.rdb1_used_for_uip_creation <= 4.5f ) {
                    if ( cl->stats.antecedents_glue_long_reds_var <= 1.05061221123f ) {
                        if ( cl->stats.sum_uip1_used <= 22.5f ) {
                            if ( cl->stats.size_rel <= 0.616987705231f ) {
                                if ( cl->stats.glue_rel_long <= 0.351752370596f ) {
                                    return 77.0/280.2;
                                } else {
                                    if ( rdb0_last_touched_diff <= 3167.0f ) {
                                        return 90.0/624.4;
                                    } else {
                                        return 103.0/452.3;
                                    }
                                }
                            } else {
                                return 106.0/284.2;
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 2333.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.124131947756f ) {
                                    if ( cl->stats.sum_uip1_used <= 111.5f ) {
                                        if ( cl->stats.used_for_uip_creation <= 8.5f ) {
                                            return 52.0/352.2;
                                        } else {
                                            return 27.0/534.3;
                                        }
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0882460400462f ) {
                                            return 15.0/418.2;
                                        } else {
                                            return 9.0/700.4;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 38.5f ) {
                                        return 41.0/548.3;
                                    } else {
                                        return 45.0/364.2;
                                    }
                                }
                            } else {
                                if ( cl->stats.dump_number <= 14.5f ) {
                                    if ( cl->stats.size_rel <= 0.26499196887f ) {
                                        return 35.0/336.2;
                                    } else {
                                        return 34.0/626.4;
                                    }
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 15201612.0f ) {
                                        return 98.0/486.3;
                                    } else {
                                        return 69.0/666.4;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.used_for_uip_creation <= 9.5f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 78.5f ) {
                                if ( rdb0_last_touched_diff <= 4083.0f ) {
                                    return 83.0/516.3;
                                } else {
                                    return 78.0/234.1;
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 2565.0f ) {
                                    return 84.0/296.2;
                                } else {
                                    return 132.0/206.1;
                                }
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 40.5f ) {
                                return 47.0/374.2;
                            } else {
                                return 30.0/474.3;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.sum_delta_confl_uip1_used <= 1395097.0f ) {
                        if ( cl->stats.sum_uip1_used <= 31.5f ) {
                            if ( cl->stats.num_overlap_literals_rel <= 0.0718203485012f ) {
                                return 68.0/480.3;
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 1614.5f ) {
                                    return 31.0/374.2;
                                } else {
                                    return 31.0/348.2;
                                }
                            }
                        } else {
                            if ( cl->size() <= 34.5f ) {
                                if ( cl->stats.size_rel <= 0.31895929575f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0276525691152f ) {
                                        return 12.0/422.2;
                                    } else {
                                        return 7.0/534.3;
                                    }
                                } else {
                                    return 39.0/676.4;
                                }
                            } else {
                                return 43.0/532.3;
                            }
                        }
                    } else {
                        if ( cl->stats.sum_uip1_used <= 167.5f ) {
                            if ( cl->stats.num_overlap_literals_rel <= 0.042605496943f ) {
                                if ( cl->stats.used_for_uip_creation <= 10.5f ) {
                                    return 51.0/426.2;
                                } else {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 19.5f ) {
                                        return 26.0/414.2;
                                    } else {
                                        return 10.0/362.2;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 4219.5f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.29336732626f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 393.5f ) {
                                            return 17.0/368.2;
                                        } else {
                                            if ( cl->size() <= 19.5f ) {
                                                return 14.0/626.4;
                                            } else {
                                                return 3.0/380.2;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.254584878683f ) {
                                            return 33.0/390.2;
                                        } else {
                                            return 12.0/386.2;
                                        }
                                    }
                                } else {
                                    return 25.0/366.2;
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.412638872862f ) {
                                if ( cl->stats.used_for_uip_creation <= 10.5f ) {
                                    return 34.0/428.2;
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.110458657146f ) {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 60834780.0f ) {
                                            return 6.0/746.4;
                                        } else {
                                            return 10.0/532.3;
                                        }
                                    } else {
                                        if ( cl->stats.dump_number <= 11.5f ) {
                                            return 18.0/488.3;
                                        } else {
                                            return 16.0/642.4;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.113986007869f ) {
                                    if ( cl->stats.dump_number <= 27.5f ) {
                                        if ( cl->stats.glue_rel_long <= 0.536274552345f ) {
                                            return 4.0/470.3;
                                        } else {
                                            return 15.0/464.3;
                                        }
                                    } else {
                                        return 19.0/462.3;
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 3919.5f ) {
                                        if ( cl->stats.glue_rel_long <= 0.753219664097f ) {
                                            if ( cl->stats.glue <= 12.5f ) {
                                                if ( cl->stats.used_for_uip_creation <= 30.5f ) {
                                                    return 5.0/528.3;
                                                } else {
                                                    return 3.0/508.3;
                                                }
                                            } else {
                                                return 0.0/536.3;
                                            }
                                        } else {
                                            if ( cl->stats.size_rel <= 0.546972036362f ) {
                                                return 11.0/398.2;
                                            } else {
                                                return 7.0/692.4;
                                            }
                                        }
                                    } else {
                                        return 15.0/442.2;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    } else {
        if ( cl->size() <= 11.5f ) {
            if ( cl->stats.glue_rel_long <= 0.618386864662f ) {
                if ( rdb0_last_touched_diff <= 71957.0f ) {
                    if ( rdb0_last_touched_diff <= 39911.0f ) {
                        if ( cl->stats.sum_uip1_used <= 66.5f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.sum_uip1_used <= 16.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.122906863689f ) {
                                        return 111.0/386.2;
                                    } else {
                                        return 94.0/228.1;
                                    }
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 1704015.5f ) {
                                        return 51.0/344.2;
                                    } else {
                                        return 102.0/306.2;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.0594735629857f ) {
                                    return 122.0/190.1;
                                } else {
                                    if ( cl->stats.size_rel <= 0.176816448569f ) {
                                        return 129.0/424.2;
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 17131.0f ) {
                                            return 98.0/290.2;
                                        } else {
                                            if ( cl->stats.dump_number <= 7.5f ) {
                                                return 102.0/260.1;
                                            } else {
                                                return 131.0/188.1;
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.420521736145f ) {
                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                    return 52.0/480.3;
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 171.0f ) {
                                        return 75.0/288.2;
                                    } else {
                                        return 32.0/398.2;
                                    }
                                }
                            } else {
                                return 52.0/688.4;
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_queue <= 0.446761250496f ) {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 9373899.0f ) {
                                if ( cl->stats.num_overlap_literals <= 9.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.076966881752f ) {
                                        if ( cl->stats.sum_uip1_used <= 18.5f ) {
                                            return 121.0/208.1;
                                        } else {
                                            return 75.0/262.1;
                                        }
                                    } else {
                                        return 99.0/350.2;
                                    }
                                } else {
                                    return 181.0/270.2;
                                }
                            } else {
                                return 110.0/586.3;
                            }
                        } else {
                            if ( cl->stats.glue <= 6.5f ) {
                                if ( cl->stats.sum_uip1_used <= 51.5f ) {
                                    if ( cl->stats.glue <= 4.5f ) {
                                        return 122.0/230.1;
                                    } else {
                                        return 228.0/294.2;
                                    }
                                } else {
                                    return 57.0/318.2;
                                }
                            } else {
                                return 93.0/306.2;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_last_touched_diff <= 142730.5f ) {
                        if ( cl->stats.glue <= 3.5f ) {
                            if ( cl->stats.num_antecedents_rel <= 0.112856402993f ) {
                                return 108.0/284.2;
                            } else {
                                return 143.0/304.2;
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.402820169926f ) {
                                if ( cl->stats.size_rel <= 0.139079123735f ) {
                                    return 113.0/250.1;
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 15.5f ) {
                                        return 187.0/98.1;
                                    } else {
                                        return 133.0/260.1;
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 6615851.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.177793949842f ) {
                                        if ( rdb0_last_touched_diff <= 108084.5f ) {
                                            return 163.0/340.2;
                                        } else {
                                            return 146.0/204.1;
                                        }
                                    } else {
                                        return 205.0/224.1;
                                    }
                                } else {
                                    return 75.0/286.2;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.dump_number <= 94.5f ) {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 20.5f ) {
                                return 268.0/136.1;
                            } else {
                                if ( cl->stats.sum_uip1_used <= 20.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.11959233135f ) {
                                        if ( cl->stats.glue_rel_long <= 0.338502079248f ) {
                                            return 164.0/86.0;
                                        } else {
                                            return 152.0/124.1;
                                        }
                                    } else {
                                        return 244.0/306.2;
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 267860.0f ) {
                                        return 183.0/396.2;
                                    } else {
                                        return 141.0/184.1;
                                    }
                                }
                            }
                        } else {
                            return 123.0/242.1;
                        }
                    }
                }
            } else {
                if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                    if ( cl->size() <= 9.5f ) {
                        if ( cl->stats.sum_uip1_used <= 5.5f ) {
                            return 182.0/334.2;
                        } else {
                            if ( rdb0_last_touched_diff <= 38310.5f ) {
                                return 57.0/424.2;
                            } else {
                                return 87.0/272.2;
                            }
                        }
                    } else {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 2543.5f ) {
                            return 178.0/122.1;
                        } else {
                            return 164.0/324.2;
                        }
                    }
                } else {
                    if ( cl->stats.num_overlap_literals <= 36.5f ) {
                        if ( cl->stats.antec_num_total_lits_rel <= 0.169694304466f ) {
                            if ( cl->stats.num_antecedents_rel <= 0.0683672279119f ) {
                                return 230.0/188.1;
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.716017246246f ) {
                                    return 162.0/314.2;
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 5184.0f ) {
                                        return 175.0/126.1;
                                    } else {
                                        return 175.0/316.2;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 9.5f ) {
                                if ( cl->size() <= 8.5f ) {
                                    return 154.0/196.1;
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.267298281193f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.242363542318f ) {
                                            return 165.0/92.1;
                                        } else {
                                            return 192.0/52.0;
                                        }
                                    } else {
                                        return 150.0/122.1;
                                    }
                                }
                            } else {
                                return 218.0/346.2;
                            }
                        }
                    } else {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 36177.5f ) {
                            if ( cl->stats.glue_rel_queue <= 0.836509227753f ) {
                                return 188.0/128.1;
                            } else {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 6.30949306488f ) {
                                    return 203.0/80.0;
                                } else {
                                    return 254.0/54.0;
                                }
                            }
                        } else {
                            return 132.0/186.1;
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.glue_rel_long <= 0.734749019146f ) {
                if ( cl->stats.sum_delta_confl_uip1_used <= 1294.0f ) {
                    if ( cl->stats.sum_uip1_used <= 2.5f ) {
                        if ( rdb0_last_touched_diff <= 49130.5f ) {
                            if ( cl->stats.glue_rel_long <= 0.51352673769f ) {
                                return 144.0/148.1;
                            } else {
                                return 332.0/162.1;
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.377380549908f ) {
                                return 164.0/80.0;
                            } else {
                                if ( rdb0_last_touched_diff <= 100661.0f ) {
                                    if ( cl->stats.glue_rel_long <= 0.657051205635f ) {
                                        return 341.0/84.0;
                                    } else {
                                        return 156.0/78.0;
                                    }
                                } else {
                                    if ( cl->stats.glue <= 7.5f ) {
                                        return 388.0/98.1;
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.231935560703f ) {
                                            if ( cl->stats.glue_rel_queue <= 0.635747373104f ) {
                                                return 183.0/46.0;
                                            } else {
                                                return 201.0/22.0;
                                            }
                                        } else {
                                            return 320.0/24.0;
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 89.5f ) {
                            if ( cl->stats.glue_rel_long <= 0.591270089149f ) {
                                return 165.0/166.1;
                            } else {
                                return 172.0/98.1;
                            }
                        } else {
                            return 179.0/84.0;
                        }
                    }
                } else {
                    if ( cl->stats.sum_uip1_used <= 27.5f ) {
                        if ( cl->stats.dump_number <= 15.5f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 55.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 24930.5f ) {
                                    return 173.0/366.2;
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.490582883358f ) {
                                        return 150.0/236.1;
                                    } else {
                                        if ( cl->stats.dump_number <= 9.5f ) {
                                            return 142.0/160.1;
                                        } else {
                                            return 147.0/146.1;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.size_rel <= 0.729503452778f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.630808115005f ) {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 42622.5f ) {
                                            return 148.0/158.1;
                                        } else {
                                            return 106.0/200.1;
                                        }
                                    } else {
                                        return 150.0/134.1;
                                    }
                                } else {
                                    return 301.0/162.1;
                                }
                            }
                        } else {
                            if ( cl->size() <= 14.5f ) {
                                if ( cl->size() <= 13.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.554951667786f ) {
                                        return 165.0/78.0;
                                    } else {
                                        return 175.0/68.0;
                                    }
                                } else {
                                    return 146.0/150.1;
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.135069981217f ) {
                                    return 324.0/80.0;
                                } else {
                                    if ( cl->stats.size_rel <= 0.807642102242f ) {
                                        if ( rdb0_last_touched_diff <= 222343.0f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.169345855713f ) {
                                                return 155.0/174.1;
                                            } else {
                                                return 194.0/106.1;
                                            }
                                        } else {
                                            return 191.0/50.0;
                                        }
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 94.5f ) {
                                            return 295.0/34.0;
                                        } else {
                                            return 195.0/60.0;
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.sum_uip1_used <= 78.5f ) {
                            if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                    return 111.0/288.2;
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.276060283184f ) {
                                        if ( cl->stats.num_overlap_literals <= 12.5f ) {
                                            return 130.0/286.2;
                                        } else {
                                            return 156.0/122.1;
                                        }
                                    } else {
                                        return 99.0/234.1;
                                    }
                                }
                            } else {
                                return 146.0/154.1;
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 116935.0f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.228131189942f ) {
                                    if ( cl->size() <= 17.5f ) {
                                        return 85.0/260.1;
                                    } else {
                                        return 90.0/486.3;
                                    }
                                } else {
                                    return 58.0/424.2;
                                }
                            } else {
                                return 162.0/330.2;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.sum_delta_confl_uip1_used <= 5128.5f ) {
                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.237187504768f ) {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 30.5f ) {
                            if ( cl->size() <= 16.5f ) {
                                return 197.0/116.1;
                            } else {
                                if ( cl->stats.glue_rel_long <= 1.30515313148f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.303090721369f ) {
                                        return 313.0/122.1;
                                    } else {
                                        return 323.0/84.0;
                                    }
                                } else {
                                    return 269.0/50.0;
                                }
                            }
                        } else {
                            return 309.0/208.1;
                        }
                    } else {
                        if ( cl->stats.glue_rel_long <= 0.928412437439f ) {
                            if ( cl->stats.size_rel <= 0.807861924171f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 155.5f ) {
                                    if ( cl->stats.glue <= 8.5f ) {
                                        return 199.0/100.1;
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.497784852982f ) {
                                            if ( cl->stats.glue_rel_queue <= 0.834028959274f ) {
                                                return 202.0/38.0;
                                            } else {
                                                return 256.0/96.1;
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 0.858821749687f ) {
                                                return 205.0/26.0;
                                            } else {
                                                return 195.0/20.0;
                                            }
                                        }
                                    }
                                } else {
                                    return 215.0/142.1;
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.907900571823f ) {
                                    if ( cl->stats.num_overlap_literals <= 75.5f ) {
                                        if ( cl->stats.num_overlap_literals <= 17.5f ) {
                                            return 219.0/46.0;
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.54099881649f ) {
                                                return 391.0/16.0;
                                            } else {
                                                return 259.0/30.0;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 1.1474956274f ) {
                                            return 291.0/36.0;
                                        } else {
                                            return 255.0/70.0;
                                        }
                                    }
                                } else {
                                    return 184.0/62.0;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 531339.0f ) {
                                if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                    if ( cl->stats.sum_uip1_used <= 1.5f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 60.5f ) {
                                            return 330.0/56.0;
                                        } else {
                                            if ( cl->stats.glue <= 13.5f ) {
                                                if ( cl->stats.glue_rel_long <= 1.08894467354f ) {
                                                    if ( cl->stats.size_rel <= 0.827117264271f ) {
                                                        return 220.0/18.0;
                                                    } else {
                                                        return 334.0/2.0;
                                                    }
                                                } else {
                                                    if ( cl->stats.num_overlap_literals_rel <= 1.12222266197f ) {
                                                        return 303.0/14.0;
                                                    } else {
                                                        return 208.0/60.0;
                                                    }
                                                }
                                            } else {
                                                if ( rdb0_last_touched_diff <= 69099.5f ) {
                                                    if ( cl->stats.antec_num_total_lits_rel <= 2.41205263138f ) {
                                                        if ( cl->stats.antecedents_glue_long_reds_var <= 49.1894226074f ) {
                                                            if ( cl->stats.num_total_lits_antecedents <= 234.5f ) {
                                                                return 393.0/26.0;
                                                            } else {
                                                                return 285.0/10.0;
                                                            }
                                                        } else {
                                                            if ( cl->stats.glue_rel_queue <= 1.23969626427f ) {
                                                                return 1;
                                                            } else {
                                                                return 184.0/4.0;
                                                            }
                                                        }
                                                    } else {
                                                        return 236.0/24.0;
                                                    }
                                                } else {
                                                    if ( cl->stats.num_overlap_literals_rel <= 1.25767827034f ) {
                                                        if ( cl->stats.size_rel <= 1.02813577652f ) {
                                                            return 327.0/4.0;
                                                        } else {
                                                            return 1;
                                                        }
                                                    } else {
                                                        if ( cl->stats.antec_num_total_lits_rel <= 1.65590417385f ) {
                                                            return 203.0/12.0;
                                                        } else {
                                                            if ( cl->stats.antec_num_total_lits_rel <= 2.48374152184f ) {
                                                                return 227.0/2.0;
                                                            } else {
                                                                return 282.0/8.0;
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.191375106573f ) {
                                            return 217.0/56.0;
                                        } else {
                                            if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                                if ( cl->size() <= 27.5f ) {
                                                    return 310.0/46.0;
                                                } else {
                                                    if ( cl->stats.glue_rel_queue <= 1.08504199982f ) {
                                                        return 222.0/18.0;
                                                    } else {
                                                        return 281.0/2.0;
                                                    }
                                                }
                                            } else {
                                                return 269.0/48.0;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 181.0f ) {
                                        if ( cl->size() <= 35.5f ) {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 5.98816585541f ) {
                                                if ( cl->stats.num_antecedents_rel <= 0.844744205475f ) {
                                                    if ( rdb0_act_ranking_top_10 <= 7.5f ) {
                                                        return 192.0/26.0;
                                                    } else {
                                                        return 198.0/50.0;
                                                    }
                                                } else {
                                                    return 254.0/100.1;
                                                }
                                            } else {
                                                if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                                    if ( cl->stats.rdb1_last_touched_diff <= 109194.5f ) {
                                                        if ( cl->stats.num_antecedents_rel <= 0.607934892178f ) {
                                                            return 202.0/28.0;
                                                        } else {
                                                            return 264.0/18.0;
                                                        }
                                                    } else {
                                                        if ( cl->stats.num_total_lits_antecedents <= 109.5f ) {
                                                            return 171.0/74.0;
                                                        } else {
                                                            return 250.0/40.0;
                                                        }
                                                    }
                                                } else {
                                                    if ( cl->stats.num_overlap_literals <= 90.5f ) {
                                                        return 264.0/4.0;
                                                    } else {
                                                        return 199.0/10.0;
                                                    }
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 356.236114502f ) {
                                                if ( rdb0_last_touched_diff <= 59137.0f ) {
                                                    return 368.0/60.0;
                                                } else {
                                                    if ( cl->stats.glue_rel_long <= 1.52583503723f ) {
                                                        if ( cl->stats.glue <= 23.5f ) {
                                                            if ( cl->stats.num_total_lits_antecedents <= 240.0f ) {
                                                                return 327.0/14.0;
                                                            } else {
                                                                return 247.0/56.0;
                                                            }
                                                        } else {
                                                            if ( cl->stats.rdb1_act_ranking_top_10 <= 7.5f ) {
                                                                return 287.0/20.0;
                                                            } else {
                                                                return 407.0/6.0;
                                                            }
                                                        }
                                                    } else {
                                                        if ( cl->stats.sum_delta_confl_uip1_used <= 2.5f ) {
                                                            return 348.0/12.0;
                                                        } else {
                                                            return 1;
                                                        }
                                                    }
                                                }
                                            } else {
                                                return 192.0/48.0;
                                            }
                                        }
                                    } else {
                                        return 310.0/98.1;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.822115302086f ) {
                                    return 164.0/88.0;
                                } else {
                                    return 185.0/60.0;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.sum_delta_confl_uip1_used <= 1492691.5f ) {
                        if ( cl->stats.dump_number <= 12.5f ) {
                            if ( cl->stats.num_overlap_literals <= 139.5f ) {
                                if ( cl->stats.dump_number <= 7.5f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 73680.0f ) {
                                        if ( cl->stats.sum_uip1_used <= 5.5f ) {
                                            return 201.0/132.1;
                                        } else {
                                            return 128.0/198.1;
                                        }
                                    } else {
                                        return 106.0/256.1;
                                    }
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 178626.0f ) {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 4.24845695496f ) {
                                            return 170.0/126.1;
                                        } else {
                                            return 186.0/42.0;
                                        }
                                    } else {
                                        return 125.0/192.1;
                                    }
                                }
                            } else {
                                return 312.0/112.1;
                            }
                        } else {
                            if ( cl->stats.dump_number <= 14.5f ) {
                                return 223.0/124.1;
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 161.5f ) {
                                    if ( cl->stats.size_rel <= 0.904725551605f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.167027264833f ) {
                                            return 210.0/66.0;
                                        } else {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.127561479807f ) {
                                                return 150.0/122.1;
                                            } else {
                                                if ( cl->stats.glue <= 9.5f ) {
                                                    return 181.0/122.1;
                                                } else {
                                                    return 264.0/116.1;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 31.5f ) {
                                            return 296.0/58.0;
                                        } else {
                                            return 250.0/76.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 26.5f ) {
                                        return 372.0/34.0;
                                    } else {
                                        return 241.0/72.0;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 181868.5f ) {
                            if ( cl->stats.glue_rel_queue <= 0.998891353607f ) {
                                if ( cl->stats.glue <= 12.5f ) {
                                    return 217.0/322.2;
                                } else {
                                    return 111.0/238.1;
                                }
                            } else {
                                if ( cl->stats.sum_uip1_used <= 48.5f ) {
                                    return 156.0/192.1;
                                } else {
                                    return 72.0/348.2;
                                }
                            }
                        } else {
                            return 216.0/108.1;
                        }
                    }
                }
            }
        }
    }
}

static double estimator_should_keep_long_conf2_cluster0_7(
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
    if ( cl->stats.sum_uip1_used <= 11.5f ) {
        if ( cl->stats.glue <= 8.5f ) {
            if ( cl->stats.rdb1_last_touched_diff <= 31410.5f ) {
                if ( cl->stats.size_rel <= 0.618511199951f ) {
                    if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                        if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                            if ( rdb0_last_touched_diff <= 10021.5f ) {
                                if ( cl->stats.size_rel <= 0.197394937277f ) {
                                    return 52.0/294.2;
                                } else {
                                    return 119.0/408.2;
                                }
                            } else {
                                if ( cl->size() <= 10.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.502170443535f ) {
                                        if ( rdb0_last_touched_diff <= 15782.0f ) {
                                            return 155.0/458.3;
                                        } else {
                                            if ( cl->stats.sum_uip1_used <= 4.5f ) {
                                                return 161.0/256.1;
                                            } else {
                                                return 97.0/250.1;
                                            }
                                        }
                                    } else {
                                        if ( cl->size() <= 6.5f ) {
                                            return 76.0/390.2;
                                        } else {
                                            if ( cl->size() <= 8.5f ) {
                                                return 174.0/366.2;
                                            } else {
                                                return 108.0/360.2;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.114635273814f ) {
                                        return 126.0/210.1;
                                    } else {
                                        if ( rdb0_last_touched_diff <= 17759.5f ) {
                                            return 149.0/142.1;
                                        } else {
                                            return 127.0/166.1;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.dump_number <= 2.5f ) {
                                return 67.0/572.3;
                            } else {
                                return 105.0/328.2;
                            }
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 17.5f ) {
                            if ( cl->stats.num_antecedents_rel <= 0.063158929348f ) {
                                return 124.0/206.1;
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.443667680025f ) {
                                    return 86.0/378.2;
                                } else {
                                    return 111.0/368.2;
                                }
                            }
                        } else {
                            if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 30583.0f ) {
                                    return 132.0/184.1;
                                } else {
                                    return 139.0/414.2;
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.351253777742f ) {
                                    return 97.0/286.2;
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.154149472713f ) {
                                        return 258.0/210.1;
                                    } else {
                                        if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                            return 214.0/372.2;
                                        } else {
                                            return 147.0/156.1;
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.142851650715f ) {
                            return 157.0/210.1;
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.582878112793f ) {
                                return 160.0/128.1;
                            } else {
                                if ( cl->size() <= 18.5f ) {
                                    return 199.0/124.1;
                                } else {
                                    return 262.0/92.1;
                                }
                            }
                        }
                    } else {
                        return 114.0/196.1;
                    }
                }
            } else {
                if ( cl->stats.size_rel <= 0.496726423502f ) {
                    if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                        if ( cl->stats.num_overlap_literals <= 12.5f ) {
                            return 106.0/306.2;
                        } else {
                            return 117.0/220.1;
                        }
                    } else {
                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                            return 150.0/268.2;
                        } else {
                            if ( rdb0_last_touched_diff <= 164736.5f ) {
                                if ( cl->stats.size_rel <= 0.169008001685f ) {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 5.5f ) {
                                        if ( cl->stats.glue <= 4.5f ) {
                                            return 140.0/272.2;
                                        } else {
                                            return 161.0/204.1;
                                        }
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.107603311539f ) {
                                            return 152.0/110.1;
                                        } else {
                                            return 100.0/176.1;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue <= 7.5f ) {
                                        if ( cl->stats.glue_rel_long <= 0.311287105083f ) {
                                            return 179.0/106.1;
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.381320536137f ) {
                                                if ( cl->stats.num_total_lits_antecedents <= 36.5f ) {
                                                    if ( cl->stats.dump_number <= 14.5f ) {
                                                        if ( cl->stats.antec_num_total_lits_rel <= 0.113776497543f ) {
                                                            return 200.0/182.1;
                                                        } else {
                                                            if ( cl->stats.glue_rel_long <= 0.593137264252f ) {
                                                                return 132.0/228.1;
                                                            } else {
                                                                return 151.0/116.1;
                                                            }
                                                        }
                                                    } else {
                                                        return 203.0/130.1;
                                                    }
                                                } else {
                                                    return 228.0/110.1;
                                                }
                                            } else {
                                                return 235.0/344.2;
                                            }
                                        }
                                    } else {
                                        return 315.0/158.1;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.480482399464f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 49.5f ) {
                                        if ( cl->stats.size_rel <= 0.131082594395f ) {
                                            return 209.0/180.1;
                                        } else {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0408498495817f ) {
                                                return 192.0/50.0;
                                            } else {
                                                if ( cl->stats.sum_delta_confl_uip1_used <= 144.5f ) {
                                                    return 279.0/102.1;
                                                } else {
                                                    return 299.0/200.1;
                                                }
                                            }
                                        }
                                    } else {
                                        return 192.0/58.0;
                                    }
                                } else {
                                    return 128.0/160.1;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.size_rel <= 0.740075826645f ) {
                        if ( cl->stats.sum_uip1_used <= 1.5f ) {
                            if ( cl->stats.dump_number <= 11.5f ) {
                                return 227.0/110.1;
                            } else {
                                return 289.0/68.0;
                            }
                        } else {
                            if ( cl->stats.dump_number <= 11.5f ) {
                                return 210.0/250.1;
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.285551518202f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 27.5f ) {
                                        return 220.0/84.0;
                                    } else {
                                        return 229.0/118.1;
                                    }
                                } else {
                                    return 168.0/150.1;
                                }
                            }
                        }
                    } else {
                        if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                            if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                if ( cl->stats.dump_number <= 6.5f ) {
                                    return 182.0/80.0;
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.420060813427f ) {
                                        if ( cl->stats.glue_rel_long <= 0.679211139679f ) {
                                            return 210.0/60.0;
                                        } else {
                                            return 215.0/16.0;
                                        }
                                    } else {
                                        return 281.0/96.1;
                                    }
                                }
                            } else {
                                return 241.0/128.1;
                            }
                        } else {
                            return 280.0/44.0;
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.glue_rel_queue <= 0.793341755867f ) {
                if ( cl->stats.size_rel <= 0.488484859467f ) {
                    if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                            return 162.0/336.2;
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.589050352573f ) {
                                return 143.0/136.1;
                            } else {
                                return 130.0/170.1;
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_long <= 0.644224047661f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 96715.5f ) {
                                return 188.0/278.2;
                            } else {
                                return 239.0/88.0;
                            }
                        } else {
                            return 300.0/128.1;
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_last_touched_diff <= 49089.5f ) {
                        if ( rdb0_last_touched_diff <= 10313.5f ) {
                            return 110.0/208.1;
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 141.5f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 65.5f ) {
                                    return 223.0/118.1;
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                        return 148.0/188.1;
                                    } else {
                                        return 241.0/192.1;
                                    }
                                }
                            } else {
                                return 281.0/108.1;
                            }
                        }
                    } else {
                        if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                            return 204.0/110.1;
                        } else {
                            if ( cl->stats.sum_uip1_used <= 6.5f ) {
                                if ( cl->stats.glue_rel_long <= 0.734610140324f ) {
                                    if ( rdb0_last_touched_diff <= 221591.0f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 128786.0f ) {
                                            return 313.0/86.0;
                                        } else {
                                            return 151.0/100.1;
                                        }
                                    } else {
                                        return 269.0/30.0;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.244646802545f ) {
                                        return 219.0/54.0;
                                    } else {
                                        return 296.0/36.0;
                                    }
                                }
                            } else {
                                return 207.0/92.1;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.sum_uip1_used <= 3.5f ) {
                    if ( cl->stats.num_total_lits_antecedents <= 39.5f ) {
                        if ( cl->size() <= 13.5f ) {
                            return 177.0/150.1;
                        } else {
                            if ( rdb0_last_touched_diff <= 55155.5f ) {
                                return 172.0/120.1;
                            } else {
                                if ( cl->size() <= 18.5f ) {
                                    return 224.0/72.0;
                                } else {
                                    return 190.0/18.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 160.5f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.0736974030733f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.426121413708f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 57490.0f ) {
                                        return 174.0/100.1;
                                    } else {
                                        return 205.0/62.0;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.404355704784f ) {
                                        return 210.0/28.0;
                                    } else {
                                        return 191.0/52.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 264.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 30002.5f ) {
                                        if ( cl->stats.glue <= 10.5f ) {
                                            return 272.0/122.1;
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 0.918800711632f ) {
                                                return 172.0/52.0;
                                            } else {
                                                if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                                        return 233.0/36.0;
                                                    } else {
                                                        if ( cl->stats.num_overlap_literals_rel <= 0.749501228333f ) {
                                                            return 258.0/4.0;
                                                        } else {
                                                            return 242.0/12.0;
                                                        }
                                                    }
                                                } else {
                                                    return 335.0/66.0;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_act_ranking_top_10 <= 8.5f ) {
                                            if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                                if ( cl->stats.glue <= 10.5f ) {
                                                    if ( cl->stats.num_overlap_literals_rel <= 0.525227963924f ) {
                                                        return 218.0/24.0;
                                                    } else {
                                                        return 292.0/76.0;
                                                    }
                                                } else {
                                                    if ( cl->stats.rdb1_last_touched_diff <= 61383.5f ) {
                                                        if ( cl->stats.size_rel <= 1.20658838749f ) {
                                                            if ( cl->size() <= 23.5f ) {
                                                                return 275.0/34.0;
                                                            } else {
                                                                return 244.0/48.0;
                                                            }
                                                        } else {
                                                            return 303.0/22.0;
                                                        }
                                                    } else {
                                                        if ( cl->stats.antec_num_total_lits_rel <= 0.698985695839f ) {
                                                            if ( cl->stats.glue <= 15.5f ) {
                                                                if ( cl->stats.glue_rel_queue <= 1.04764437675f ) {
                                                                    return 269.0/8.0;
                                                                } else {
                                                                    return 283.0/2.0;
                                                                }
                                                            } else {
                                                                if ( rdb0_last_touched_diff <= 153415.0f ) {
                                                                    return 224.0/10.0;
                                                                } else {
                                                                    return 188.0/16.0;
                                                                }
                                                            }
                                                        } else {
                                                            if ( cl->stats.rdb1_act_ranking_top_10 <= 5.5f ) {
                                                                if ( cl->stats.rdb1_last_touched_diff <= 94201.5f ) {
                                                                    return 238.0/32.0;
                                                                } else {
                                                                    if ( cl->stats.sum_delta_confl_uip1_used <= 1.5f ) {
                                                                        return 210.0/2.0;
                                                                    } else {
                                                                        return 289.0/6.0;
                                                                    }
                                                                }
                                                            } else {
                                                                return 219.0/52.0;
                                                            }
                                                        }
                                                    }
                                                }
                                            } else {
                                                if ( rdb0_last_touched_diff <= 99493.0f ) {
                                                    return 282.0/26.0;
                                                } else {
                                                    if ( cl->stats.num_antecedents_rel <= 0.728171408176f ) {
                                                        if ( cl->stats.dump_number <= 30.5f ) {
                                                            if ( cl->stats.num_overlap_literals_rel <= 0.349091112614f ) {
                                                                return 180.0/40.0;
                                                            } else {
                                                                return 256.0/28.0;
                                                            }
                                                        } else {
                                                            return 198.0/58.0;
                                                        }
                                                    } else {
                                                        if ( cl->stats.num_antecedents_rel <= 1.03308773041f ) {
                                                            return 169.0/98.1;
                                                        } else {
                                                            return 199.0/68.0;
                                                        }
                                                    }
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 132.5f ) {
                                                if ( cl->stats.size_rel <= 1.27276945114f ) {
                                                    if ( cl->stats.rdb1_last_touched_diff <= 189043.0f ) {
                                                        return 202.0/10.0;
                                                    } else {
                                                        return 214.0/4.0;
                                                    }
                                                } else {
                                                    return 192.0/32.0;
                                                }
                                            } else {
                                                if ( cl->stats.antecedents_glue_long_reds_var <= 20.15625f ) {
                                                    return 1;
                                                } else {
                                                    return 208.0/4.0;
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 45.5f ) {
                                        if ( cl->stats.size_rel <= 0.244066059589f ) {
                                            return 255.0/52.0;
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 115701.0f ) {
                                                if ( cl->size() <= 197.5f ) {
                                                    if ( cl->stats.glue_rel_queue <= 1.50528645515f ) {
                                                        if ( cl->size() <= 28.5f ) {
                                                            if ( cl->stats.antec_num_total_lits_rel <= 2.20648908615f ) {
                                                                return 237.0/16.0;
                                                            } else {
                                                                return 230.0/32.0;
                                                            }
                                                        } else {
                                                            if ( cl->stats.size_rel <= 2.04608011246f ) {
                                                                if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                                                    if ( cl->stats.sum_delta_confl_uip1_used <= 3.5f ) {
                                                                        if ( cl->stats.glue_rel_queue <= 1.04558777809f ) {
                                                                            return 238.0/14.0;
                                                                        } else {
                                                                            if ( cl->stats.num_total_lits_antecedents <= 716.5f ) {
                                                                                return 1;
                                                                            } else {
                                                                                return 222.0/8.0;
                                                                            }
                                                                        }
                                                                    } else {
                                                                        return 364.0/2.0;
                                                                    }
                                                                } else {
                                                                    return 274.0/16.0;
                                                                }
                                                            } else {
                                                                return 200.0/14.0;
                                                            }
                                                        }
                                                    } else {
                                                        return 380.0/46.0;
                                                    }
                                                } else {
                                                    if ( cl->stats.size_rel <= 1.92132115364f ) {
                                                        return 316.0/70.0;
                                                    } else {
                                                        if ( cl->stats.num_overlap_literals_rel <= 1.35186767578f ) {
                                                            return 222.0/4.0;
                                                        } else {
                                                            return 200.0/20.0;
                                                        }
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.num_antecedents_rel <= 2.43238997459f ) {
                                                    if ( cl->stats.num_overlap_literals <= 316.5f ) {
                                                        if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                                            return 267.0/2.0;
                                                        } else {
                                                            return 270.0/14.0;
                                                        }
                                                    } else {
                                                        if ( cl->stats.glue_rel_long <= 1.13399100304f ) {
                                                            return 189.0/2.0;
                                                        } else {
                                                            return 1;
                                                        }
                                                    }
                                                } else {
                                                    if ( cl->stats.glue <= 22.5f ) {
                                                        return 203.0/16.0;
                                                    } else {
                                                        return 253.0/6.0;
                                                    }
                                                }
                                            }
                                        }
                                    } else {
                                        return 187.0/42.0;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.size_rel <= 0.822061896324f ) {
                                if ( cl->stats.dump_number <= 7.5f ) {
                                    return 252.0/180.1;
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.670857429504f ) {
                                        return 229.0/86.0;
                                    } else {
                                        return 155.0/92.1;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue <= 16.5f ) {
                                    if ( cl->stats.dump_number <= 16.5f ) {
                                        return 293.0/56.0;
                                    } else {
                                        return 247.0/12.0;
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.526069700718f ) {
                                        return 277.0/58.0;
                                    } else {
                                        return 233.0/94.1;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.0381944440305f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 69543.0f ) {
                                return 149.0/250.1;
                            } else {
                                return 206.0/78.0;
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 28767.5f ) {
                                if ( cl->stats.dump_number <= 2.5f ) {
                                    return 167.0/108.1;
                                } else {
                                    return 241.0/120.1;
                                }
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 38252.0f ) {
                                    if ( cl->stats.glue_rel_long <= 0.950997948647f ) {
                                        return 308.0/120.1;
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 13.2453451157f ) {
                                            return 250.0/46.0;
                                        } else {
                                            return 346.0/22.0;
                                        }
                                    }
                                } else {
                                    if ( cl->size() <= 21.5f ) {
                                        return 287.0/160.1;
                                    } else {
                                        if ( cl->stats.dump_number <= 21.5f ) {
                                            return 235.0/98.1;
                                        } else {
                                            return 250.0/42.0;
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.num_overlap_literals_rel <= 0.339917302132f ) {
                            return 141.0/282.2;
                        } else {
                            return 147.0/142.1;
                        }
                    }
                }
            }
        }
    } else {
        if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
            if ( cl->stats.sum_uip1_used <= 62.5f ) {
                if ( cl->size() <= 10.5f ) {
                    if ( rdb0_last_touched_diff <= 42096.0f ) {
                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                            if ( cl->stats.glue_rel_queue <= 0.450350880623f ) {
                                if ( cl->stats.glue_rel_long <= 0.377965152264f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 2140426.5f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.119453594089f ) {
                                            return 64.0/320.2;
                                        } else {
                                            return 42.0/380.2;
                                        }
                                    } else {
                                        return 101.0/350.2;
                                    }
                                } else {
                                    return 96.0/310.2;
                                }
                            } else {
                                if ( cl->stats.dump_number <= 13.5f ) {
                                    if ( cl->stats.dump_number <= 7.5f ) {
                                        return 28.0/442.2;
                                    } else {
                                        return 31.0/346.2;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.57144510746f ) {
                                        return 69.0/272.2;
                                    } else {
                                        return 70.0/310.2;
                                    }
                                }
                            }
                        } else {
                            if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                                return 239.0/594.3;
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.0983445346355f ) {
                                    return 86.0/272.2;
                                } else {
                                    return 69.0/334.2;
                                }
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 112014.0f ) {
                            if ( cl->stats.glue_rel_long <= 0.543804109097f ) {
                                if ( cl->stats.dump_number <= 33.5f ) {
                                    if ( cl->stats.sum_uip1_used <= 20.5f ) {
                                        return 118.0/240.1;
                                    } else {
                                        return 134.0/562.3;
                                    }
                                } else {
                                    return 154.0/180.1;
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                    return 180.0/180.1;
                                } else {
                                    return 114.0/192.1;
                                }
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 20.5f ) {
                                return 236.0/242.1;
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.13737128675f ) {
                                    return 205.0/368.2;
                                } else {
                                    return 146.0/188.1;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.dump_number <= 19.5f ) {
                        if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 47947.0f ) {
                                if ( cl->stats.sum_uip1_used <= 29.5f ) {
                                    if ( cl->size() <= 23.5f ) {
                                        if ( cl->stats.glue_rel_long <= 0.615009665489f ) {
                                            return 99.0/346.2;
                                        } else {
                                            return 136.0/284.2;
                                        }
                                    } else {
                                        return 229.0/310.2;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.597288668156f ) {
                                        return 65.0/306.2;
                                    } else {
                                        return 92.0/276.2;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 123.5f ) {
                                    if ( cl->stats.sum_uip1_used <= 17.5f ) {
                                        return 188.0/150.1;
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.245000004768f ) {
                                            return 91.0/226.1;
                                        } else {
                                            return 125.0/224.1;
                                        }
                                    }
                                } else {
                                    return 178.0/114.1;
                                }
                            }
                        } else {
                            if ( cl->stats.dump_number <= 8.5f ) {
                                if ( rdb0_last_touched_diff <= 1805.5f ) {
                                    return 33.0/340.2;
                                } else {
                                    return 86.0/384.2;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 20.5f ) {
                                    return 53.0/290.2;
                                } else {
                                    return 90.0/250.1;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                            if ( cl->size() <= 20.5f ) {
                                return 173.0/342.2;
                            } else {
                                return 142.0/194.1;
                            }
                        } else {
                            if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 62.5f ) {
                                    if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                        return 306.0/188.1;
                                    } else {
                                        if ( cl->stats.sum_uip1_used <= 32.5f ) {
                                            if ( cl->stats.dump_number <= 34.5f ) {
                                                return 127.0/146.1;
                                            } else {
                                                return 240.0/148.1;
                                            }
                                        } else {
                                            return 141.0/224.1;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.368299901485f ) {
                                        return 319.0/142.1;
                                    } else {
                                        return 277.0/210.1;
                                    }
                                }
                            } else {
                                return 173.0/286.2;
                            }
                        }
                    }
                }
            } else {
                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                    if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                        if ( rdb0_last_touched_diff <= 40110.0f ) {
                            if ( cl->stats.size_rel <= 0.33394318819f ) {
                                if ( cl->stats.glue_rel_long <= 0.330933153629f ) {
                                    return 46.0/342.2;
                                } else {
                                    return 14.0/456.3;
                                }
                            } else {
                                return 68.0/520.3;
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.447455465794f ) {
                                return 58.0/344.2;
                            } else {
                                return 78.0/256.1;
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_queue <= 0.832702994347f ) {
                            if ( cl->stats.num_overlap_literals_rel <= 0.029533745721f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0185051187873f ) {
                                    return 47.0/596.3;
                                } else {
                                    return 53.0/460.3;
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.379512369633f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.093718111515f ) {
                                        return 26.0/398.2;
                                    } else {
                                        return 17.0/568.3;
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 1144.5f ) {
                                        return 23.0/656.4;
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 26533.0f ) {
                                            return 50.0/660.4;
                                        } else {
                                            return 38.0/322.2;
                                        }
                                    }
                                }
                            }
                        } else {
                            return 48.0/426.2;
                        }
                    }
                } else {
                    if ( cl->stats.num_overlap_literals_rel <= 0.0315647833049f ) {
                        if ( cl->stats.dump_number <= 15.5f ) {
                            return 54.0/368.2;
                        } else {
                            if ( rdb0_last_touched_diff <= 121467.0f ) {
                                if ( cl->stats.dump_number <= 33.5f ) {
                                    return 100.0/256.1;
                                } else {
                                    return 111.0/382.2;
                                }
                            } else {
                                return 215.0/318.2;
                            }
                        }
                    } else {
                        if ( cl->stats.antec_num_total_lits_rel <= 0.18103697896f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 27.5f ) {
                                if ( cl->stats.dump_number <= 42.5f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 3995111.0f ) {
                                        return 24.0/424.2;
                                    } else {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 9856311.0f ) {
                                            return 59.0/294.2;
                                        } else {
                                            return 42.0/528.3;
                                        }
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 97601.5f ) {
                                        return 103.0/586.3;
                                    } else {
                                        return 99.0/264.1;
                                    }
                                }
                            } else {
                                if ( cl->stats.dump_number <= 29.5f ) {
                                    return 56.0/292.2;
                                } else {
                                    return 101.0/280.2;
                                }
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 27.5f ) {
                                return 55.0/338.2;
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 29540.5f ) {
                                    return 70.0/508.3;
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.79480111599f ) {
                                        if ( cl->stats.sum_uip1_used <= 99.5f ) {
                                            return 127.0/174.1;
                                        } else {
                                            return 119.0/334.2;
                                        }
                                    } else {
                                        return 74.0/292.2;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.sum_uip1_used <= 58.5f ) {
                if ( cl->stats.dump_number <= 18.5f ) {
                    if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 115.0f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 8147.5f ) {
                                if ( cl->stats.sum_uip1_used <= 18.5f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0340432487428f ) {
                                        return 108.0/226.1;
                                    } else {
                                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                            return 71.0/452.3;
                                        } else {
                                            return 96.0/298.2;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 10.5f ) {
                                        if ( cl->stats.glue_rel_long <= 0.662245750427f ) {
                                            if ( cl->stats.num_total_lits_antecedents <= 27.5f ) {
                                                if ( cl->stats.rdb1_last_touched_diff <= 3058.5f ) {
                                                    return 28.0/602.3;
                                                } else {
                                                    return 48.0/476.3;
                                                }
                                            } else {
                                                return 65.0/504.3;
                                            }
                                        } else {
                                            return 71.0/440.2;
                                        }
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 21.5f ) {
                                            return 53.0/398.2;
                                        } else {
                                            return 95.0/402.2;
                                        }
                                    }
                                }
                            } else {
                                return 125.0/442.2;
                            }
                        } else {
                            return 193.0/398.2;
                        }
                    } else {
                        if ( cl->stats.glue <= 11.5f ) {
                            if ( rdb0_last_touched_diff <= 943.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 4256.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.0936550721526f ) {
                                        return 39.0/506.3;
                                    } else {
                                        if ( rdb0_last_touched_diff <= 468.0f ) {
                                            return 12.0/662.4;
                                        } else {
                                            return 16.0/424.2;
                                        }
                                    }
                                } else {
                                    return 33.0/460.3;
                                }
                            } else {
                                if ( cl->stats.glue <= 3.5f ) {
                                    if ( cl->stats.size_rel <= 0.112411141396f ) {
                                        return 13.0/478.3;
                                    } else {
                                        return 32.0/488.3;
                                    }
                                } else {
                                    if ( cl->size() <= 16.5f ) {
                                        if ( cl->stats.size_rel <= 0.203397274017f ) {
                                            if ( cl->stats.num_total_lits_antecedents <= 18.5f ) {
                                                return 66.0/454.3;
                                            } else {
                                                return 31.0/384.2;
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 836.5f ) {
                                                return 22.0/550.3;
                                            } else {
                                                if ( cl->stats.size_rel <= 0.389420449734f ) {
                                                    if ( cl->stats.sum_uip1_used <= 28.5f ) {
                                                        return 30.0/354.2;
                                                    } else {
                                                        return 25.0/380.2;
                                                    }
                                                } else {
                                                    return 56.0/456.3;
                                                }
                                            }
                                        }
                                    } else {
                                        return 97.0/520.3;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.681325256824f ) {
                                return 21.0/354.2;
                            } else {
                                return 99.0/472.3;
                            }
                        }
                    }
                } else {
                    if ( cl->size() <= 16.5f ) {
                        if ( cl->stats.sum_uip1_used <= 24.5f ) {
                            return 156.0/310.2;
                        } else {
                            if ( rdb0_last_touched_diff <= 8098.5f ) {
                                return 85.0/500.3;
                            } else {
                                return 167.0/452.3;
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 10159.5f ) {
                            return 101.0/222.1;
                        } else {
                            return 183.0/212.1;
                        }
                    }
                }
            } else {
                if ( cl->stats.rdb1_last_touched_diff <= 3163.5f ) {
                    if ( cl->stats.num_overlap_literals <= 5.5f ) {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 6.5f ) {
                            if ( cl->stats.glue_rel_queue <= 0.420420587063f ) {
                                if ( cl->stats.sum_uip1_used <= 143.5f ) {
                                    return 48.0/528.3;
                                } else {
                                    if ( rdb0_last_touched_diff <= 3471.5f ) {
                                        return 4.0/460.3;
                                    } else {
                                        return 12.0/382.2;
                                    }
                                }
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                    return 55.0/456.3;
                                } else {
                                    return 22.0/416.2;
                                }
                            }
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                                return 33.0/646.4;
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.454747259617f ) {
                                    if ( cl->stats.used_for_uip_creation <= 10.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 471.0f ) {
                                            return 5.0/442.2;
                                        } else {
                                            return 2.0/848.5;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 12.5f ) {
                                            return 11.0/480.3;
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.0330425351858f ) {
                                                if ( cl->stats.sum_delta_confl_uip1_used <= 20101552.0f ) {
                                                    return 7.0/454.3;
                                                } else {
                                                    return 13.0/400.2;
                                                }
                                            } else {
                                                if ( cl->stats.num_antecedents_rel <= 0.142525106668f ) {
                                                    if ( cl->stats.used_for_uip_creation <= 52.5f ) {
                                                        if ( cl->stats.sum_uip1_used <= 365.5f ) {
                                                            return 11.0/508.3;
                                                        } else {
                                                            return 1.0/428.2;
                                                        }
                                                    } else {
                                                        return 2.0/610.3;
                                                    }
                                                } else {
                                                    return 0.0/550.3;
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue <= 9.5f ) {
                                        if ( cl->stats.glue_rel_long <= 0.540793061256f ) {
                                            return 20.0/542.3;
                                        } else {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 20894972.0f ) {
                                                return 16.0/404.2;
                                            } else {
                                                return 5.0/462.3;
                                            }
                                        }
                                    } else {
                                        return 6.0/502.3;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 197517312.0f ) {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0572955608368f ) {
                                    return 68.0/320.2;
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 127.5f ) {
                                        return 62.0/392.2;
                                    } else {
                                        return 30.0/546.3;
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 5322.0f ) {
                                    if ( cl->stats.dump_number <= 24.5f ) {
                                        if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                            return 32.0/516.3;
                                        } else {
                                            if ( cl->stats.size_rel <= 0.219180345535f ) {
                                                if ( cl->stats.rdb1_last_touched_diff <= 81.5f ) {
                                                    return 0.0/538.3;
                                                } else {
                                                    if ( cl->stats.glue_rel_queue <= 0.579567432404f ) {
                                                        if ( cl->stats.size_rel <= 0.121438354254f ) {
                                                            if ( cl->stats.sum_delta_confl_uip1_used <= 10970760.0f ) {
                                                                return 22.0/664.4;
                                                            } else {
                                                                return 5.0/554.3;
                                                            }
                                                        } else {
                                                            if ( cl->stats.num_overlap_literals_rel <= 0.0578888729215f ) {
                                                                return 4.0/414.2;
                                                            } else {
                                                                return 0.0/514.3;
                                                            }
                                                        }
                                                    } else {
                                                        return 16.0/458.3;
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.size_rel <= 0.538685917854f ) {
                                                    if ( cl->stats.dump_number <= 8.5f ) {
                                                        if ( cl->stats.glue_rel_queue <= 0.428606510162f ) {
                                                            return 38.0/462.3;
                                                        } else {
                                                            if ( cl->stats.used_for_uip_creation <= 35.5f ) {
                                                                return 19.0/408.2;
                                                            } else {
                                                                return 3.0/390.2;
                                                            }
                                                        }
                                                    } else {
                                                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.149444445968f ) {
                                                            return 6.0/630.4;
                                                        } else {
                                                            return 15.0/406.2;
                                                        }
                                                    }
                                                } else {
                                                    if ( cl->stats.sum_uip1_used <= 115.0f ) {
                                                        return 16.0/378.2;
                                                    } else {
                                                        if ( cl->stats.used_for_uip_creation <= 45.5f ) {
                                                            return 9.0/476.3;
                                                        } else {
                                                            return 2.0/466.3;
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.sum_uip1_used <= 232.5f ) {
                                            return 47.0/516.3;
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 1205.0f ) {
                                                if ( cl->stats.sum_delta_confl_uip1_used <= 82590432.0f ) {
                                                    return 0.0/398.2;
                                                } else {
                                                    return 8.0/386.2;
                                                }
                                            } else {
                                                return 21.0/378.2;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 0.491325110197f ) {
                                        if ( cl->stats.glue_rel_long <= 0.501450538635f ) {
                                            if ( cl->size() <= 7.5f ) {
                                                return 21.0/640.4;
                                            } else {
                                                return 22.0/530.3;
                                            }
                                        } else {
                                            return 45.0/492.3;
                                        }
                                    } else {
                                        return 57.0/458.3;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals_rel <= 0.128884732723f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 30.5f ) {
                                    return 11.0/682.4;
                                } else {
                                    return 24.0/414.2;
                                }
                            } else {
                                return 3.0/624.4;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.dump_number <= 20.5f ) {
                        if ( cl->stats.sum_uip1_used <= 172.5f ) {
                            if ( cl->stats.num_antecedents_rel <= 0.0559709481895f ) {
                                return 45.0/498.3;
                            } else {
                                if ( cl->stats.size_rel <= 0.570621967316f ) {
                                    if ( cl->stats.used_for_uip_creation <= 7.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.103690370917f ) {
                                            return 42.0/540.3;
                                        } else {
                                            return 28.0/652.4;
                                        }
                                    } else {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 3766581.0f ) {
                                            return 15.0/460.3;
                                        } else {
                                            return 3.0/408.2;
                                        }
                                    }
                                } else {
                                    return 40.0/402.2;
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.265786111355f ) {
                                return 22.0/408.2;
                            } else {
                                if ( cl->stats.dump_number <= 9.5f ) {
                                    return 4.0/442.2;
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 27.5f ) {
                                        return 8.0/444.3;
                                    } else {
                                        return 18.0/396.2;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.sum_uip1_used <= 267.5f ) {
                            if ( rdb0_last_touched_diff <= 9804.0f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0868234634399f ) {
                                    if ( cl->size() <= 5.5f ) {
                                        return 22.0/454.3;
                                    } else {
                                        return 50.0/634.4;
                                    }
                                } else {
                                    return 71.0/610.3;
                                }
                            } else {
                                if ( cl->stats.size_rel <= 0.249143242836f ) {
                                    return 103.0/298.2;
                                } else {
                                    return 119.0/474.3;
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.241434067488f ) {
                                return 30.0/342.2;
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.410455465317f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 12.5f ) {
                                        return 2.0/456.3;
                                    } else {
                                        return 16.0/490.3;
                                    }
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 465.0f ) {
                                        return 33.0/480.3;
                                    } else {
                                        return 21.0/654.4;
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

static double estimator_should_keep_long_conf2_cluster0_8(
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
    if ( cl->stats.sum_delta_confl_uip1_used <= 4226.0f ) {
        if ( cl->size() <= 10.5f ) {
            if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                if ( rdb0_last_touched_diff <= 10717.0f ) {
                    return 32.0/364.2;
                } else {
                    if ( cl->stats.glue_rel_queue <= 0.823272109032f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 4689.0f ) {
                            return 74.0/282.2;
                        } else {
                            if ( cl->size() <= 6.5f ) {
                                return 77.0/334.2;
                            } else {
                                return 174.0/320.2;
                            }
                        }
                    } else {
                        return 130.0/192.1;
                    }
                }
            } else {
                if ( cl->stats.glue_rel_long <= 0.723771512508f ) {
                    if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 146477.0f ) {
                            if ( rdb0_last_touched_diff <= 50736.0f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 23469.0f ) {
                                    return 157.0/266.2;
                                } else {
                                    return 92.0/244.1;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0677063316107f ) {
                                    return 108.0/198.1;
                                } else {
                                    return 187.0/244.1;
                                }
                            }
                        } else {
                            return 148.0/132.1;
                        }
                    } else {
                        if ( cl->size() <= 6.5f ) {
                            return 223.0/264.1;
                        } else {
                            if ( cl->stats.num_overlap_literals_rel <= 0.0878101289272f ) {
                                return 130.0/114.1;
                            } else {
                                return 215.0/126.1;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                        return 152.0/146.1;
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 65.5f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 29.5f ) {
                                return 251.0/192.1;
                            } else {
                                return 135.0/134.1;
                            }
                        } else {
                            return 318.0/80.0;
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.antecedents_glue_long_reds_var <= 0.323098391294f ) {
                if ( rdb0_last_touched_diff <= 68722.0f ) {
                    if ( cl->stats.glue_rel_long <= 0.693988800049f ) {
                        if ( cl->stats.sum_uip1_used <= 2.5f ) {
                            if ( cl->stats.glue_rel_long <= 0.466091215611f ) {
                                return 108.0/214.1;
                            } else {
                                return 180.0/134.1;
                            }
                        } else {
                            return 125.0/310.2;
                        }
                    } else {
                        if ( cl->stats.sum_uip1_used <= 2.5f ) {
                            if ( cl->stats.size_rel <= 0.788548529148f ) {
                                return 312.0/206.1;
                            } else {
                                return 344.0/124.1;
                            }
                        } else {
                            return 159.0/166.1;
                        }
                    }
                } else {
                    if ( cl->stats.num_overlap_literals <= 4.5f ) {
                        return 205.0/94.1;
                    } else {
                        if ( cl->stats.num_antecedents_rel <= 0.534404098988f ) {
                            if ( rdb0_last_touched_diff <= 202900.0f ) {
                                if ( cl->stats.size_rel <= 0.684680461884f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.680521309376f ) {
                                        return 195.0/104.1;
                                    } else {
                                        return 209.0/46.0;
                                    }
                                } else {
                                    return 329.0/60.0;
                                }
                            } else {
                                if ( cl->stats.glue <= 8.5f ) {
                                    return 203.0/16.0;
                                } else {
                                    return 340.0/68.0;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 181926.0f ) {
                                return 232.0/104.1;
                            } else {
                                return 256.0/88.0;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.num_overlap_literals <= 49.5f ) {
                    if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                        if ( cl->stats.glue_rel_queue <= 0.817478477955f ) {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 5.5f ) {
                                return 249.0/124.1;
                            } else {
                                return 200.0/328.2;
                            }
                        } else {
                            if ( cl->stats.glue <= 11.5f ) {
                                return 338.0/140.1;
                            } else {
                                if ( rdb0_last_touched_diff <= 35742.5f ) {
                                    return 244.0/30.0;
                                } else {
                                    return 215.0/34.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_queue <= 0.557326257229f ) {
                            return 244.0/142.1;
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.981132626534f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.102415755391f ) {
                                    return 225.0/28.0;
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 20.5f ) {
                                        if ( cl->stats.size_rel <= 0.691554188728f ) {
                                            return 174.0/104.1;
                                        } else {
                                            return 352.0/90.1;
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 31.5f ) {
                                            if ( cl->stats.num_antecedents_rel <= 0.325792849064f ) {
                                                return 206.0/48.0;
                                            } else {
                                                return 255.0/28.0;
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 151306.5f ) {
                                                return 263.0/122.1;
                                            } else {
                                                return 258.0/20.0;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.glue <= 18.5f ) {
                                    if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 1.16151070595f ) {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 10.6133337021f ) {
                                                return 350.0/62.0;
                                            } else {
                                                return 215.0/24.0;
                                            }
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 61.5f ) {
                                                return 219.0/82.0;
                                            } else {
                                                return 226.0/62.0;
                                            }
                                        }
                                    } else {
                                        return 308.0/24.0;
                                    }
                                } else {
                                    return 355.0/22.0;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.glue_rel_queue <= 0.855023622513f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 55956.0f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 343.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 140.5f ) {
                                    return 233.0/126.1;
                                } else {
                                    return 267.0/90.1;
                                }
                            } else {
                                return 179.0/120.1;
                            }
                        } else {
                            if ( cl->stats.glue <= 14.5f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 91.5f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 4.7863535881f ) {
                                        return 295.0/106.1;
                                    } else {
                                        return 293.0/38.0;
                                    }
                                } else {
                                    return 157.0/76.0;
                                }
                            } else {
                                return 252.0/32.0;
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_long <= 1.02442133427f ) {
                            if ( cl->size() <= 21.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 10.7090272903f ) {
                                    if ( cl->stats.glue <= 9.5f ) {
                                        return 200.0/60.0;
                                    } else {
                                        return 288.0/26.0;
                                    }
                                } else {
                                    return 355.0/144.1;
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.902198672295f ) {
                                    return 329.0/76.0;
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 158.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.836180448532f ) {
                                            if ( cl->stats.size_rel <= 1.09177517891f ) {
                                                return 362.0/16.0;
                                            } else {
                                                return 248.0/2.0;
                                            }
                                        } else {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 3.5f ) {
                                                if ( cl->stats.glue_rel_long <= 0.942854106426f ) {
                                                    return 194.0/46.0;
                                                } else {
                                                    return 258.0/30.0;
                                                }
                                            } else {
                                                return 279.0/10.0;
                                            }
                                        }
                                    } else {
                                        return 198.0/48.0;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 5.5f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 87.5f ) {
                                    if ( cl->stats.num_overlap_literals <= 147.5f ) {
                                        if ( cl->stats.glue_rel_long <= 1.12921977043f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 54426.0f ) {
                                                return 220.0/14.0;
                                            } else {
                                                return 221.0/6.0;
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 1.26473093033f ) {
                                                if ( cl->stats.num_overlap_literals_rel <= 0.983999729156f ) {
                                                    return 289.0/28.0;
                                                } else {
                                                    return 171.0/50.0;
                                                }
                                            } else {
                                                if ( cl->stats.size_rel <= 1.4451444149f ) {
                                                    if ( cl->stats.num_antecedents_rel <= 0.695060253143f ) {
                                                        return 238.0/4.0;
                                                    } else {
                                                        return 227.0/18.0;
                                                    }
                                                } else {
                                                    return 200.0/28.0;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 1.83264136314f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 5.35454463959f ) {
                                                if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                                                    if ( cl->stats.num_antecedents_rel <= 0.706610321999f ) {
                                                        if ( cl->stats.antecedents_glue_long_reds_var <= 48.2010765076f ) {
                                                            return 194.0/16.0;
                                                        } else {
                                                            return 317.0/6.0;
                                                        }
                                                    } else {
                                                        if ( cl->stats.num_overlap_literals <= 305.0f ) {
                                                            if ( cl->stats.size_rel <= 1.27615714073f ) {
                                                                return 363.0/16.0;
                                                            } else {
                                                                return 1;
                                                            }
                                                        } else {
                                                            if ( cl->stats.rdb1_last_touched_diff <= 128920.5f ) {
                                                                return 1;
                                                            } else {
                                                                return 200.0/2.0;
                                                            }
                                                        }
                                                    }
                                                } else {
                                                    return 272.0/16.0;
                                                }
                                            } else {
                                                return 205.0/20.0;
                                            }
                                        } else {
                                            return 203.0/26.0;
                                        }
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 56625.0f ) {
                                        return 178.0/54.0;
                                    } else {
                                        return 225.0/24.0;
                                    }
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                    if ( rdb0_last_touched_diff <= 369278.0f ) {
                                        if ( cl->stats.num_overlap_literals <= 113.5f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 93581.0f ) {
                                                return 192.0/26.0;
                                            } else {
                                                return 271.0/78.0;
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 60136.5f ) {
                                                return 309.0/74.0;
                                            } else {
                                                if ( cl->stats.sum_delta_confl_uip1_used <= 6.5f ) {
                                                    if ( cl->stats.size_rel <= 1.3880366087f ) {
                                                        return 374.0/8.0;
                                                    } else {
                                                        return 276.0/30.0;
                                                    }
                                                } else {
                                                    return 268.0/36.0;
                                                }
                                            }
                                        }
                                    } else {
                                        return 242.0/100.1;
                                    }
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 15.5f ) {
                                        if ( cl->stats.num_overlap_literals <= 356.0f ) {
                                            if ( cl->stats.num_antecedents_rel <= 1.38081932068f ) {
                                                if ( cl->stats.glue_rel_long <= 1.49126935005f ) {
                                                    return 327.0/28.0;
                                                } else {
                                                    return 1;
                                                }
                                            } else {
                                                return 187.0/22.0;
                                            }
                                        } else {
                                            return 312.0/2.0;
                                        }
                                    } else {
                                        return 203.0/26.0;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    } else {
        if ( cl->stats.rdb1_last_touched_diff <= 26812.5f ) {
            if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                    if ( cl->stats.sum_uip1_used <= 16.5f ) {
                        if ( cl->stats.glue_rel_queue <= 0.881233751774f ) {
                            if ( cl->size() <= 10.5f ) {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                                    if ( cl->stats.dump_number <= 12.5f ) {
                                        if ( cl->stats.sum_uip1_used <= 3.5f ) {
                                            return 136.0/292.2;
                                        } else {
                                            if ( cl->stats.dump_number <= 4.5f ) {
                                                if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                                    return 50.0/304.2;
                                                } else {
                                                    return 69.0/294.2;
                                                }
                                            } else {
                                                if ( cl->stats.size_rel <= 0.255291461945f ) {
                                                    return 93.0/340.2;
                                                } else {
                                                    return 93.0/230.1;
                                                }
                                            }
                                        }
                                    } else {
                                        return 220.0/306.2;
                                    }
                                } else {
                                    return 136.0/218.1;
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 174.5f ) {
                                    if ( cl->stats.size_rel <= 0.432992935181f ) {
                                        if ( cl->stats.glue_rel_long <= 0.453982532024f ) {
                                            return 98.0/256.1;
                                        } else {
                                            return 187.0/320.2;
                                        }
                                    } else {
                                        if ( cl->stats.dump_number <= 13.5f ) {
                                            if ( cl->stats.num_overlap_literals <= 28.5f ) {
                                                if ( cl->stats.num_antecedents_rel <= 0.168573558331f ) {
                                                    return 134.0/190.1;
                                                } else {
                                                    return 190.0/352.2;
                                                }
                                            } else {
                                                return 226.0/188.1;
                                            }
                                        } else {
                                            return 226.0/146.1;
                                        }
                                    }
                                } else {
                                    return 208.0/106.1;
                                }
                            }
                        } else {
                            if ( cl->stats.dump_number <= 7.5f ) {
                                if ( cl->stats.size_rel <= 0.506933271885f ) {
                                    return 170.0/254.1;
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 5.3213262558f ) {
                                        return 116.0/152.1;
                                    } else {
                                        return 149.0/104.1;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 32.5f ) {
                                    return 178.0/154.1;
                                } else {
                                    return 240.0/104.1;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.sum_uip1_used <= 131.5f ) {
                            if ( cl->stats.dump_number <= 19.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 29.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 11656.5f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.162570536137f ) {
                                            if ( cl->stats.dump_number <= 8.5f ) {
                                                return 35.0/430.2;
                                            } else {
                                                return 92.0/508.3;
                                            }
                                        } else {
                                            return 42.0/516.3;
                                        }
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.137342065573f ) {
                                            return 135.0/486.3;
                                        } else {
                                            return 51.0/390.2;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.174516111612f ) {
                                        if ( cl->stats.num_overlap_literals <= 16.5f ) {
                                            return 45.0/318.2;
                                        } else {
                                            return 68.0/334.2;
                                        }
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.180521562696f ) {
                                            return 154.0/360.2;
                                        } else {
                                            if ( cl->stats.dump_number <= 10.5f ) {
                                                return 100.0/476.3;
                                            } else {
                                                return 83.0/268.2;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 60.5f ) {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                        if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                                            if ( cl->stats.dump_number <= 39.5f ) {
                                                if ( cl->stats.rdb1_last_touched_diff <= 12003.0f ) {
                                                    return 165.0/484.3;
                                                } else {
                                                    return 68.0/330.2;
                                                }
                                            } else {
                                                if ( cl->stats.sum_delta_confl_uip1_used <= 13524022.0f ) {
                                                    return 130.0/192.1;
                                                } else {
                                                    return 136.0/336.2;
                                                }
                                            }
                                        } else {
                                            return 83.0/366.2;
                                        }
                                    } else {
                                        return 207.0/332.2;
                                    }
                                } else {
                                    return 192.0/292.2;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 11406.5f ) {
                                if ( cl->stats.sum_uip1_used <= 358.0f ) {
                                    if ( rdb0_last_touched_diff <= 10633.5f ) {
                                        return 53.0/520.3;
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.393263429403f ) {
                                            return 57.0/328.2;
                                        } else {
                                            return 80.0/352.2;
                                        }
                                    }
                                } else {
                                    return 34.0/700.4;
                                }
                            } else {
                                if ( cl->stats.glue <= 5.5f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0501825399697f ) {
                                        return 48.0/494.3;
                                    } else {
                                        return 15.0/386.2;
                                    }
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 31876818.0f ) {
                                        return 20.0/396.2;
                                    } else {
                                        return 47.0/346.2;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( rdb0_last_touched_diff <= 5890.0f ) {
                        if ( cl->stats.size_rel <= 0.626866817474f ) {
                            if ( cl->stats.glue <= 6.5f ) {
                                if ( cl->size() <= 7.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 12487.0f ) {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.149444445968f ) {
                                            if ( cl->stats.num_antecedents_rel <= 0.0506400205195f ) {
                                                return 55.0/532.3;
                                            } else {
                                                if ( cl->stats.num_overlap_literals <= 6.5f ) {
                                                    if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                                        return 52.0/342.2;
                                                    } else {
                                                        if ( cl->stats.sum_delta_confl_uip1_used <= 26849082.0f ) {
                                                            if ( cl->stats.size_rel <= 0.200211048126f ) {
                                                                return 22.0/568.3;
                                                            } else {
                                                                return 37.0/408.2;
                                                            }
                                                        } else {
                                                            return 10.0/420.2;
                                                        }
                                                    }
                                                } else {
                                                    return 26.0/724.4;
                                                }
                                            }
                                        } else {
                                            return 18.0/634.4;
                                        }
                                    } else {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 19985000.0f ) {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 559746.5f ) {
                                                return 65.0/322.2;
                                            } else {
                                                return 81.0/618.3;
                                            }
                                        } else {
                                            return 15.0/378.2;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 579604.0f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 9331.5f ) {
                                            return 49.0/372.2;
                                        } else {
                                            return 84.0/258.1;
                                        }
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.0821233093739f ) {
                                            return 52.0/360.2;
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 12787.0f ) {
                                                return 32.0/602.3;
                                            } else {
                                                return 35.0/360.2;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.0624764934182f ) {
                                    return 100.0/342.2;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 6065.0f ) {
                                        if ( cl->stats.glue_rel_long <= 0.64512270689f ) {
                                            return 27.0/504.3;
                                        } else {
                                            return 45.0/384.2;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.614078342915f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 13127.0f ) {
                                                return 40.0/346.2;
                                            } else {
                                                return 26.0/388.2;
                                            }
                                        } else {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.269999980927f ) {
                                                return 66.0/392.2;
                                            } else {
                                                return 100.0/308.2;
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.543599128723f ) {
                                if ( cl->stats.dump_number <= 10.5f ) {
                                    return 136.0/500.3;
                                } else {
                                    return 84.0/594.3;
                                }
                            } else {
                                return 107.0/350.2;
                            }
                        }
                    } else {
                        if ( cl->stats.num_overlap_literals <= 29.5f ) {
                            if ( cl->size() <= 13.5f ) {
                                if ( cl->stats.sum_uip1_used <= 53.5f ) {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                        return 99.0/554.3;
                                    } else {
                                        return 87.0/298.2;
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 27.5f ) {
                                        return 11.0/372.2;
                                    } else {
                                        return 42.0/418.2;
                                    }
                                }
                            } else {
                                return 141.0/394.2;
                            }
                        } else {
                            return 168.0/352.2;
                        }
                    }
                }
            } else {
                if ( rdb0_last_touched_diff <= 9092.5f ) {
                    if ( rdb0_last_touched_diff <= 2612.5f ) {
                        if ( cl->size() <= 5.5f ) {
                            if ( cl->stats.glue_rel_queue <= 0.178141713142f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 609.5f ) {
                                    return 6.0/486.3;
                                } else {
                                    return 34.0/558.3;
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 854.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.52682363987f ) {
                                        if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                            if ( rdb0_last_touched_diff <= 860.5f ) {
                                                if ( cl->stats.rdb1_last_touched_diff <= 264.5f ) {
                                                    return 8.0/416.2;
                                                } else {
                                                    return 14.0/384.2;
                                                }
                                            } else {
                                                return 7.0/530.3;
                                            }
                                        } else {
                                            if ( cl->stats.num_overlap_literals <= 3.5f ) {
                                                return 4.0/446.3;
                                            } else {
                                                return 13.0/744.4;
                                            }
                                        }
                                    } else {
                                        return 18.0/400.2;
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 14.5f ) {
                                        if ( cl->stats.glue_rel_long <= 0.252703309059f ) {
                                            return 8.0/372.2;
                                        } else {
                                            if ( cl->stats.sum_uip1_used <= 86.5f ) {
                                                return 11.0/478.3;
                                            } else {
                                                if ( cl->stats.rdb1_used_for_uip_creation <= 13.5f ) {
                                                    return 0.0/750.4;
                                                } else {
                                                    return 3.0/756.4;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.118426606059f ) {
                                            return 14.0/472.3;
                                        } else {
                                            return 7.0/474.3;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 20.5f ) {
                                return 99.0/542.3;
                            } else {
                                if ( rdb0_last_touched_diff <= 927.5f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.00571291195229f ) {
                                        return 25.0/488.3;
                                    } else {
                                        if ( cl->stats.used_for_uip_creation <= 27.5f ) {
                                            if ( cl->stats.num_total_lits_antecedents <= 30.5f ) {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.0813251882792f ) {
                                                    return 23.0/692.4;
                                                } else {
                                                    if ( cl->stats.dump_number <= 10.5f ) {
                                                        return 5.0/562.3;
                                                    } else {
                                                        return 2.0/592.3;
                                                    }
                                                }
                                            } else {
                                                if ( cl->size() <= 18.5f ) {
                                                    return 45.0/522.3;
                                                } else {
                                                    if ( cl->stats.sum_uip1_used <= 110.5f ) {
                                                        return 32.0/552.3;
                                                    } else {
                                                        if ( cl->stats.num_total_lits_antecedents <= 87.5f ) {
                                                            return 2.0/448.3;
                                                        } else {
                                                            return 6.0/432.2;
                                                        }
                                                    }
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 2.51302099228f ) {
                                                if ( cl->stats.num_total_lits_antecedents <= 36.5f ) {
                                                    if ( cl->size() <= 7.5f ) {
                                                        return 18.0/524.3;
                                                    } else {
                                                        if ( cl->stats.num_antecedents_rel <= 0.0974673479795f ) {
                                                            return 3.0/442.2;
                                                        } else {
                                                            return 11.0/568.3;
                                                        }
                                                    }
                                                } else {
                                                    if ( cl->stats.sum_uip1_used <= 196.5f ) {
                                                        return 7.0/448.3;
                                                    } else {
                                                        if ( cl->stats.rdb1_used_for_uip_creation <= 111.5f ) {
                                                            if ( cl->stats.dump_number <= 17.5f ) {
                                                                return 0.0/550.3;
                                                            } else {
                                                                return 2.0/406.2;
                                                            }
                                                        } else {
                                                            return 4.0/408.2;
                                                        }
                                                    }
                                                }
                                            } else {
                                                return 15.0/374.2;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                        return 53.0/584.3;
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0474868193269f ) {
                                            if ( cl->stats.dump_number <= 6.5f ) {
                                                return 37.0/482.3;
                                            } else {
                                                if ( cl->stats.rdb1_used_for_uip_creation <= 18.5f ) {
                                                    return 43.0/570.3;
                                                } else {
                                                    return 10.0/452.3;
                                                }
                                            }
                                        } else {
                                            if ( rdb0_last_touched_diff <= 1211.5f ) {
                                                return 43.0/468.3;
                                            } else {
                                                if ( cl->stats.used_for_uip_creation <= 17.5f ) {
                                                    if ( cl->stats.rdb1_last_touched_diff <= 1155.0f ) {
                                                        return 18.0/410.2;
                                                    } else {
                                                        return 14.0/550.3;
                                                    }
                                                } else {
                                                    return 12.0/840.5;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue <= 4.5f ) {
                            if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 11.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 2496.5f ) {
                                        return 42.0/422.2;
                                    } else {
                                        return 56.0/388.2;
                                    }
                                } else {
                                    return 20.0/388.2;
                                }
                            } else {
                                if ( cl->size() <= 4.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.0838671326637f ) {
                                        return 26.0/706.4;
                                    } else {
                                        if ( cl->stats.size_rel <= 0.13590246439f ) {
                                            return 2.0/438.2;
                                        } else {
                                            return 6.0/384.2;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.108310252428f ) {
                                        return 24.0/610.3;
                                    } else {
                                        return 39.0/378.2;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 425999.0f ) {
                                if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                    return 117.0/364.2;
                                } else {
                                    return 58.0/534.3;
                                }
                            } else {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 6.5f ) {
                                    if ( cl->stats.sum_uip1_used <= 55.5f ) {
                                        return 65.0/340.2;
                                    } else {
                                        return 39.0/704.4;
                                    }
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.228298604488f ) {
                                        if ( rdb0_last_touched_diff <= 7454.5f ) {
                                            if ( cl->stats.sum_uip1_used <= 77.5f ) {
                                                return 48.0/440.2;
                                            } else {
                                                if ( cl->stats.num_overlap_literals_rel <= 0.0255150049925f ) {
                                                    return 23.0/374.2;
                                                } else {
                                                    if ( cl->stats.sum_uip1_used <= 165.0f ) {
                                                        return 22.0/400.2;
                                                    } else {
                                                        return 15.0/698.4;
                                                    }
                                                }
                                            }
                                        } else {
                                            return 44.0/348.2;
                                        }
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.166305184364f ) {
                                            return 36.0/608.3;
                                        } else {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.337825387716f ) {
                                                return 10.0/422.2;
                                            } else {
                                                return 12.0/346.2;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.sum_delta_confl_uip1_used <= 946693.5f ) {
                        if ( cl->stats.num_overlap_literals_rel <= 0.222664028406f ) {
                            if ( cl->stats.sum_uip1_used <= 37.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.113308668137f ) {
                                    return 169.0/438.2;
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.163234621286f ) {
                                        return 51.0/392.2;
                                    } else {
                                        return 73.0/296.2;
                                    }
                                }
                            } else {
                                return 28.0/378.2;
                            }
                        } else {
                            return 164.0/398.2;
                        }
                    } else {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 13.5f ) {
                            if ( cl->stats.sum_uip1_used <= 84.5f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.134236127138f ) {
                                    return 96.0/526.3;
                                } else {
                                    return 137.0/412.2;
                                }
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 72863648.0f ) {
                                    if ( rdb0_last_touched_diff <= 14106.0f ) {
                                        return 49.0/680.4;
                                    } else {
                                        return 58.0/420.2;
                                    }
                                } else {
                                    return 12.0/422.2;
                                }
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals_rel <= 0.0335332453251f ) {
                                return 48.0/566.3;
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 39.5f ) {
                                    return 21.0/650.4;
                                } else {
                                    return 31.0/362.2;
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.num_total_lits_antecedents <= 25.5f ) {
                if ( cl->stats.num_total_lits_antecedents <= 17.5f ) {
                    if ( cl->stats.rdb1_last_touched_diff <= 80917.0f ) {
                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                            if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0492480397224f ) {
                                    if ( cl->stats.sum_uip1_used <= 39.5f ) {
                                        return 78.0/252.1;
                                    } else {
                                        return 41.0/388.2;
                                    }
                                } else {
                                    return 84.0/266.2;
                                }
                            } else {
                                return 44.0/510.3;
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.641267538071f ) {
                                if ( cl->size() <= 4.5f ) {
                                    if ( rdb0_last_touched_diff <= 55126.0f ) {
                                        return 59.0/320.2;
                                    } else {
                                        return 87.0/254.1;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.463148117065f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0713457614183f ) {
                                            return 133.0/216.1;
                                        } else {
                                            return 134.0/484.3;
                                        }
                                    } else {
                                        return 106.0/360.2;
                                    }
                                }
                            } else {
                                return 143.0/214.1;
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 97771.0f ) {
                            return 148.0/440.2;
                        } else {
                            if ( cl->stats.sum_uip1_used <= 11.5f ) {
                                if ( cl->stats.size_rel <= 0.191788583994f ) {
                                    return 146.0/162.1;
                                } else {
                                    return 179.0/128.1;
                                }
                            } else {
                                if ( cl->stats.sum_uip1_used <= 51.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.459318369627f ) {
                                        return 158.0/310.2;
                                    } else {
                                        return 130.0/144.1;
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.0885924696922f ) {
                                        return 108.0/218.1;
                                    } else {
                                        return 100.0/362.2;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.glue_rel_queue <= 0.682556986809f ) {
                        if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                            if ( rdb0_last_touched_diff <= 109946.0f ) {
                                if ( cl->stats.sum_uip1_used <= 21.5f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 133582.5f ) {
                                        return 103.0/234.1;
                                    } else {
                                        return 222.0/176.1;
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 22.5f ) {
                                        if ( cl->stats.num_overlap_literals <= 7.5f ) {
                                            return 77.0/298.2;
                                        } else {
                                            return 60.0/296.2;
                                        }
                                    } else {
                                        return 94.0/260.1;
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_uip1_used <= 21.5f ) {
                                    return 293.0/192.1;
                                } else {
                                    return 203.0/396.2;
                                }
                            }
                        } else {
                            return 132.0/552.3;
                        }
                    } else {
                        if ( cl->stats.num_overlap_literals_rel <= 0.0503963492811f ) {
                            return 155.0/158.1;
                        } else {
                            if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                return 118.0/256.1;
                            } else {
                                return 170.0/150.1;
                            }
                        }
                    }
                }
            } else {
                if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                    if ( cl->stats.size_rel <= 0.697792470455f ) {
                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 1081668.5f ) {
                                return 220.0/294.2;
                            } else {
                                return 113.0/384.2;
                            }
                        } else {
                            if ( cl->stats.dump_number <= 25.5f ) {
                                if ( cl->stats.num_overlap_literals <= 55.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.694023728371f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.0830240249634f ) {
                                            return 176.0/172.1;
                                        } else {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 304639.0f ) {
                                                if ( cl->stats.size_rel <= 0.421792715788f ) {
                                                    if ( cl->stats.num_overlap_literals <= 19.5f ) {
                                                        return 84.0/244.1;
                                                    } else {
                                                        return 121.0/210.1;
                                                    }
                                                } else {
                                                    return 266.0/244.1;
                                                }
                                            } else {
                                                if ( cl->stats.size_rel <= 0.326473712921f ) {
                                                    return 105.0/244.1;
                                                } else {
                                                    return 118.0/402.2;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.sum_uip1_used <= 15.5f ) {
                                            if ( cl->stats.num_antecedents_rel <= 0.22132691741f ) {
                                                return 237.0/80.0;
                                            } else {
                                                return 194.0/188.1;
                                            }
                                        } else {
                                            return 98.0/260.1;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 18.5f ) {
                                        if ( cl->stats.num_overlap_literals <= 165.5f ) {
                                            return 292.0/182.1;
                                        } else {
                                            return 166.0/60.0;
                                        }
                                    } else {
                                        return 101.0/218.1;
                                    }
                                }
                            } else {
                                if ( cl->size() <= 10.5f ) {
                                    if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                        return 192.0/330.2;
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.19418990612f ) {
                                            return 162.0/140.1;
                                        } else {
                                            return 141.0/192.1;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 96.5f ) {
                                        if ( cl->stats.dump_number <= 83.5f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 243309.5f ) {
                                                if ( cl->stats.rdb1_act_ranking_top_10 <= 5.5f ) {
                                                    if ( cl->stats.sum_uip1_used <= 29.5f ) {
                                                        return 229.0/94.1;
                                                    } else {
                                                        return 112.0/220.1;
                                                    }
                                                } else {
                                                    return 202.0/134.1;
                                                }
                                            } else {
                                                if ( cl->stats.glue_rel_long <= 0.62964630127f ) {
                                                    return 163.0/96.1;
                                                } else {
                                                    return 203.0/72.0;
                                                }
                                            }
                                        } else {
                                            return 204.0/240.1;
                                        }
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.403818964958f ) {
                                            return 185.0/62.0;
                                        } else {
                                            return 155.0/94.1;
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.size_rel <= 1.19281554222f ) {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 1652945.5f ) {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                                    if ( cl->stats.num_overlap_literals <= 33.5f ) {
                                        return 165.0/136.1;
                                    } else {
                                        return 168.0/110.1;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.736681759357f ) {
                                        if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                            return 189.0/152.1;
                                        } else {
                                            return 240.0/94.1;
                                        }
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.173749998212f ) {
                                            return 165.0/98.1;
                                        } else {
                                            if ( cl->stats.sum_uip1_used <= 7.5f ) {
                                                if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                                    return 292.0/64.0;
                                                } else {
                                                    return 227.0/80.0;
                                                }
                                            } else {
                                                return 205.0/102.1;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 8276305.5f ) {
                                    return 246.0/254.1;
                                } else {
                                    return 105.0/228.1;
                                }
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.102040819824f ) {
                                return 184.0/142.1;
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 90.5f ) {
                                    return 206.0/106.1;
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 518197.0f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.483457356691f ) {
                                            return 239.0/8.0;
                                        } else {
                                            return 286.0/42.0;
                                        }
                                    } else {
                                        return 174.0/98.1;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.0273303389549f ) {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 1620463.5f ) {
                            return 125.0/322.2;
                        } else {
                            return 54.0/498.3;
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 1266.0f ) {
                            return 108.0/378.2;
                        } else {
                            if ( cl->stats.sum_uip1_used <= 27.5f ) {
                                if ( cl->stats.num_overlap_literals <= 35.5f ) {
                                    return 202.0/236.1;
                                } else {
                                    return 277.0/184.1;
                                }
                            } else {
                                return 90.0/278.2;
                            }
                        }
                    }
                }
            }
        }
    }
}

static double estimator_should_keep_long_conf2_cluster0_9(
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
    if ( cl->stats.antecedents_glue_long_reds_var <= 2.2516541481f ) {
        if ( rdb0_act_ranking_top_10 <= 2.5f ) {
            if ( cl->stats.rdb1_last_touched_diff <= 11404.0f ) {
                if ( cl->stats.sum_delta_confl_uip1_used <= 218128.5f ) {
                    if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                        if ( cl->size() <= 10.5f ) {
                            if ( cl->stats.sum_uip1_used <= 10.5f ) {
                                if ( cl->size() <= 4.5f ) {
                                    return 59.0/302.2;
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 49840.0f ) {
                                        if ( rdb0_last_touched_diff <= 11895.5f ) {
                                            return 73.0/366.2;
                                        } else {
                                            return 166.0/472.3;
                                        }
                                    } else {
                                        return 102.0/200.1;
                                    }
                                }
                            } else {
                                return 62.0/506.3;
                            }
                        } else {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 321.5f ) {
                                if ( cl->stats.num_overlap_literals <= 24.5f ) {
                                    return 162.0/144.1;
                                } else {
                                    return 174.0/78.0;
                                }
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 12313.5f ) {
                                    return 95.0/272.2;
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 6.5f ) {
                                        return 184.0/178.1;
                                    } else {
                                        return 120.0/378.2;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.210284665227f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 1802.0f ) {
                                if ( rdb0_last_touched_diff <= 826.5f ) {
                                    return 7.0/432.2;
                                } else {
                                    if ( cl->stats.size_rel <= 0.221616834402f ) {
                                        return 29.0/362.2;
                                    } else {
                                        return 28.0/388.2;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.449978798628f ) {
                                    return 39.0/448.3;
                                } else {
                                    return 91.0/442.2;
                                }
                            }
                        } else {
                            if ( cl->size() <= 12.5f ) {
                                return 60.0/492.3;
                            } else {
                                return 86.0/250.1;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 4.5f ) {
                        if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.339677035809f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 8291654.0f ) {
                                    if ( cl->stats.dump_number <= 19.5f ) {
                                        if ( cl->stats.size_rel <= 0.153352156281f ) {
                                            return 82.0/680.4;
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 0.601288497448f ) {
                                                if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                                    return 62.0/408.2;
                                                } else {
                                                    if ( cl->stats.sum_uip1_used <= 31.5f ) {
                                                        return 103.0/248.1;
                                                    } else {
                                                        return 42.0/376.2;
                                                    }
                                                }
                                            } else {
                                                return 137.0/432.2;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.sum_uip1_used <= 19.5f ) {
                                            return 142.0/170.1;
                                        } else {
                                            return 196.0/428.2;
                                        }
                                    }
                                } else {
                                    if ( cl->size() <= 12.5f ) {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 126458448.0f ) {
                                            if ( cl->stats.num_total_lits_antecedents <= 13.5f ) {
                                                if ( cl->stats.glue_rel_queue <= 0.334035873413f ) {
                                                    return 41.0/344.2;
                                                } else {
                                                    return 35.0/420.2;
                                                }
                                            } else {
                                                if ( cl->stats.dump_number <= 47.5f ) {
                                                    return 61.0/530.3;
                                                } else {
                                                    return 89.0/300.2;
                                                }
                                            }
                                        } else {
                                            return 16.0/422.2;
                                        }
                                    } else {
                                        return 123.0/380.2;
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_uip1_used <= 36.5f ) {
                                    return 234.0/320.2;
                                } else {
                                    return 87.0/534.3;
                                }
                            }
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                return 119.0/582.3;
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 21.5f ) {
                                    if ( cl->stats.sum_uip1_used <= 29.5f ) {
                                        return 67.0/506.3;
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0332392826676f ) {
                                            if ( cl->stats.glue_rel_queue <= 0.29970061779f ) {
                                                return 43.0/464.3;
                                            } else {
                                                if ( cl->stats.used_for_uip_creation <= 8.5f ) {
                                                    return 28.0/356.2;
                                                } else {
                                                    return 8.0/408.2;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 0.313177198172f ) {
                                                return 4.0/406.2;
                                            } else {
                                                if ( cl->stats.sum_uip1_used <= 137.5f ) {
                                                    return 28.0/680.4;
                                                } else {
                                                    return 14.0/658.4;
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.16470438242f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 46.5f ) {
                                            if ( cl->stats.glue_rel_queue <= 0.40858399868f ) {
                                                return 23.0/406.2;
                                            } else {
                                                return 58.0/470.3;
                                            }
                                        } else {
                                            return 74.0/332.2;
                                        }
                                    } else {
                                        if ( cl->stats.size_rel <= 0.737293601036f ) {
                                            if ( cl->stats.sum_uip1_used <= 101.5f ) {
                                                return 49.0/668.4;
                                            } else {
                                                return 11.0/466.3;
                                            }
                                        } else {
                                            return 59.0/422.2;
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.sum_uip1_used <= 58.5f ) {
                            if ( rdb0_last_touched_diff <= 6253.5f ) {
                                if ( cl->stats.size_rel <= 0.300819128752f ) {
                                    if ( cl->stats.sum_uip1_used <= 27.5f ) {
                                        return 37.0/396.2;
                                    } else {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 11.5f ) {
                                            if ( cl->stats.num_overlap_literals <= 5.5f ) {
                                                return 16.0/390.2;
                                            } else {
                                                return 29.0/416.2;
                                            }
                                        } else {
                                            return 25.0/814.5;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 38.5f ) {
                                        return 69.0/502.3;
                                    } else {
                                        return 42.0/674.4;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.407461881638f ) {
                                    return 57.0/482.3;
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0516728051007f ) {
                                        return 84.0/280.2;
                                    } else {
                                        return 92.0/518.3;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 2755.0f ) {
                                if ( cl->size() <= 5.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 13.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 1610.5f ) {
                                            if ( rdb0_last_touched_diff <= 6591.5f ) {
                                                if ( cl->stats.dump_number <= 8.5f ) {
                                                    if ( cl->stats.num_antecedents_rel <= 0.0459511056542f ) {
                                                        return 4.0/426.2;
                                                    } else {
                                                        return 0.0/658.4;
                                                    }
                                                } else {
                                                    if ( cl->stats.used_for_uip_creation <= 60.5f ) {
                                                        if ( cl->stats.rdb1_used_for_uip_creation <= 9.5f ) {
                                                            return 7.0/426.2;
                                                        } else {
                                                            if ( cl->stats.used_for_uip_creation <= 26.5f ) {
                                                                if ( cl->stats.sum_uip1_used <= 314.0f ) {
                                                                    return 2.0/422.2;
                                                                } else {
                                                                    return 0.0/612.3;
                                                                }
                                                            } else {
                                                                return 3.0/406.2;
                                                            }
                                                        }
                                                    } else {
                                                        return 9.0/402.2;
                                                    }
                                                }
                                            } else {
                                                return 12.0/400.2;
                                            }
                                        } else {
                                            return 16.0/744.4;
                                        }
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.0453744567931f ) {
                                            return 26.0/402.2;
                                        } else {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 12.5f ) {
                                                return 21.0/440.2;
                                            } else {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.077859826386f ) {
                                                    return 12.0/450.3;
                                                } else {
                                                    if ( cl->stats.num_overlap_literals_rel <= 0.12142381072f ) {
                                                        return 4.0/430.2;
                                                    } else {
                                                        return 6.0/410.2;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 24.5f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0517903864384f ) {
                                                return 51.0/466.3;
                                            } else {
                                                return 39.0/638.4;
                                            }
                                        } else {
                                            return 10.0/440.2;
                                        }
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.00963452830911f ) {
                                            return 29.0/378.2;
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.57480430603f ) {
                                                if ( cl->stats.glue_rel_queue <= 0.326732039452f ) {
                                                    if ( cl->stats.glue_rel_queue <= 0.290592133999f ) {
                                                        if ( cl->stats.sum_delta_confl_uip1_used <= 39464484.0f ) {
                                                            if ( cl->stats.used_for_uip_creation <= 19.5f ) {
                                                                return 12.0/424.2;
                                                            } else {
                                                                return 7.0/642.4;
                                                            }
                                                        } else {
                                                            return 24.0/584.3;
                                                        }
                                                    } else {
                                                        return 27.0/396.2;
                                                    }
                                                } else {
                                                    if ( cl->stats.rdb1_used_for_uip_creation <= 7.5f ) {
                                                        return 24.0/598.3;
                                                    } else {
                                                        if ( cl->stats.size_rel <= 0.376665979624f ) {
                                                            if ( cl->size() <= 8.5f ) {
                                                                if ( cl->stats.num_total_lits_antecedents <= 17.5f ) {
                                                                    return 6.0/570.3;
                                                                } else {
                                                                    return 27.0/582.3;
                                                                }
                                                            } else {
                                                                if ( cl->stats.rdb1_last_touched_diff <= 152.5f ) {
                                                                    return 14.0/582.3;
                                                                } else {
                                                                    if ( cl->stats.num_total_lits_antecedents <= 51.5f ) {
                                                                        return 6.0/778.4;
                                                                    } else {
                                                                        return 0.0/436.2;
                                                                    }
                                                                }
                                                            }
                                                        } else {
                                                            if ( cl->stats.num_overlap_literals <= 24.5f ) {
                                                                if ( cl->stats.sum_uip1_used <= 182.5f ) {
                                                                    return 36.0/696.4;
                                                                } else {
                                                                    if ( cl->stats.rdb1_last_touched_diff <= 234.0f ) {
                                                                        return 8.0/478.3;
                                                                    } else {
                                                                        return 0.0/616.3;
                                                                    }
                                                                }
                                                            } else {
                                                                return 24.0/416.2;
                                                            }
                                                        }
                                                    }
                                                }
                                            } else {
                                                return 6.0/736.4;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.738741993904f ) {
                                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                        if ( cl->stats.sum_uip1_used <= 695.0f ) {
                                            if ( rdb0_last_touched_diff <= 3622.0f ) {
                                                if ( cl->stats.used_for_uip_creation <= 5.5f ) {
                                                    return 22.0/502.3;
                                                } else {
                                                    if ( cl->stats.sum_uip1_used <= 253.5f ) {
                                                        if ( cl->stats.glue_rel_long <= 0.435787200928f ) {
                                                            return 15.0/684.4;
                                                        } else {
                                                            return 15.0/350.2;
                                                        }
                                                    } else {
                                                        return 3.0/758.4;
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.0802728533745f ) {
                                                    return 58.0/428.2;
                                                } else {
                                                    return 41.0/684.4;
                                                }
                                            }
                                        } else {
                                            return 6.0/668.4;
                                        }
                                    } else {
                                        return 31.0/368.2;
                                    }
                                } else {
                                    return 59.0/624.4;
                                }
                            }
                        }
                    }
                }
            } else {
                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                    if ( cl->stats.sum_delta_confl_uip1_used <= 271469.5f ) {
                        if ( cl->stats.num_overlap_literals_rel <= 0.209917157888f ) {
                            if ( cl->stats.size_rel <= 0.702564239502f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 20940.5f ) {
                                    if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                        return 95.0/318.2;
                                    } else {
                                        return 75.0/404.2;
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 0.148394823074f ) {
                                        return 113.0/406.2;
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 29483.5f ) {
                                            return 89.0/250.1;
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 0.575902581215f ) {
                                                return 140.0/176.1;
                                            } else {
                                                return 99.0/204.1;
                                            }
                                        }
                                    }
                                }
                            } else {
                                return 153.0/142.1;
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 3.5f ) {
                                return 199.0/138.1;
                            } else {
                                return 106.0/162.1;
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 3386.0f ) {
                            if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                if ( rdb0_last_touched_diff <= 610.5f ) {
                                    return 47.0/382.2;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 35764.5f ) {
                                        return 118.0/554.3;
                                    } else {
                                        return 119.0/334.2;
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_uip1_used <= 56.5f ) {
                                    if ( cl->stats.size_rel <= 0.310343235731f ) {
                                        return 46.0/512.3;
                                    } else {
                                        return 82.0/436.2;
                                    }
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 13.5f ) {
                                        if ( cl->stats.num_overlap_literals <= 5.5f ) {
                                            return 12.0/384.2;
                                        } else {
                                            return 42.0/448.3;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.390634775162f ) {
                                            return 8.0/400.2;
                                        } else {
                                            return 4.0/516.3;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 10545898.0f ) {
                                if ( cl->stats.size_rel <= 0.624794006348f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.223870247602f ) {
                                        if ( cl->stats.dump_number <= 22.5f ) {
                                            if ( cl->stats.num_total_lits_antecedents <= 15.5f ) {
                                                if ( cl->stats.num_overlap_literals_rel <= 0.0232417732477f ) {
                                                    return 48.0/332.2;
                                                } else {
                                                    return 28.0/400.2;
                                                }
                                            } else {
                                                if ( cl->stats.size_rel <= 0.324245274067f ) {
                                                    return 137.0/480.3;
                                                } else {
                                                    return 60.0/460.3;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 24158.0f ) {
                                                return 97.0/272.2;
                                            } else {
                                                return 163.0/314.2;
                                            }
                                        }
                                    } else {
                                        return 130.0/248.1;
                                    }
                                } else {
                                    return 224.0/260.1;
                                }
                            } else {
                                if ( cl->stats.sum_uip1_used <= 81.0f ) {
                                    return 109.0/202.1;
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.204861104488f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.115694776177f ) {
                                            return 62.0/520.3;
                                        } else {
                                            if ( cl->stats.dump_number <= 61.5f ) {
                                                return 15.0/434.2;
                                            } else {
                                                return 51.0/336.2;
                                            }
                                        }
                                    } else {
                                        return 50.0/330.2;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.size_rel <= 0.772910952568f ) {
                        if ( cl->stats.sum_uip1_used <= 29.5f ) {
                            if ( cl->size() <= 10.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 40836.0f ) {
                                    if ( cl->stats.dump_number <= 7.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0744208693504f ) {
                                            return 70.0/280.2;
                                        } else {
                                            return 95.0/238.1;
                                        }
                                    } else {
                                        return 207.0/318.2;
                                    }
                                } else {
                                    return 252.0/340.2;
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 57791.0f ) {
                                    if ( cl->stats.glue_rel_long <= 0.464846611023f ) {
                                        return 105.0/186.1;
                                    } else {
                                        if ( cl->stats.sum_uip1_used <= 6.5f ) {
                                            return 196.0/116.1;
                                        } else {
                                            return 134.0/184.1;
                                        }
                                    }
                                } else {
                                    return 241.0/116.1;
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 61100.5f ) {
                                if ( cl->size() <= 9.5f ) {
                                    if ( cl->stats.sum_uip1_used <= 137.5f ) {
                                        if ( cl->stats.dump_number <= 37.5f ) {
                                            return 102.0/490.3;
                                        } else {
                                            return 114.0/194.1;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 25960.5f ) {
                                            return 33.0/484.3;
                                        } else {
                                            return 41.0/378.2;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 0.423552274704f ) {
                                        return 89.0/274.2;
                                    } else {
                                        return 56.0/280.2;
                                    }
                                }
                            } else {
                                return 151.0/376.2;
                            }
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 80.0f ) {
                            return 198.0/206.1;
                        } else {
                            return 220.0/82.0;
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.sum_delta_confl_uip1_used <= 1588.0f ) {
                if ( cl->stats.glue_rel_long <= 0.476113140583f ) {
                    if ( rdb0_last_touched_diff <= 75266.5f ) {
                        if ( cl->size() <= 6.5f ) {
                            return 105.0/284.2;
                        } else {
                            return 218.0/262.1;
                        }
                    } else {
                        if ( cl->stats.dump_number <= 28.5f ) {
                            if ( cl->stats.dump_number <= 14.5f ) {
                                return 255.0/186.1;
                            } else {
                                return 225.0/208.1;
                            }
                        } else {
                            return 182.0/56.0;
                        }
                    }
                } else {
                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.0279145445675f ) {
                        if ( cl->stats.rdb1_act_ranking_top_10 <= 8.5f ) {
                            if ( cl->stats.dump_number <= 9.5f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 44.5f ) {
                                    if ( cl->stats.dump_number <= 2.5f ) {
                                        return 170.0/156.1;
                                    } else {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 2.5f ) {
                                            if ( cl->stats.glue <= 8.5f ) {
                                                return 156.0/172.1;
                                            } else {
                                                return 187.0/66.0;
                                            }
                                        } else {
                                            return 206.0/100.1;
                                        }
                                    }
                                } else {
                                    return 172.0/188.1;
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 24.5f ) {
                                    return 176.0/102.1;
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 14.5f ) {
                                        return 274.0/52.0;
                                    } else {
                                        return 305.0/104.1;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals <= 19.5f ) {
                                return 327.0/112.1;
                            } else {
                                return 237.0/44.0;
                            }
                        }
                    } else {
                        if ( cl->stats.num_overlap_literals_rel <= 1.52092444897f ) {
                            if ( cl->size() <= 10.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 74322.5f ) {
                                    return 138.0/164.1;
                                } else {
                                    return 154.0/108.1;
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 62762.0f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.322218596935f ) {
                                        return 291.0/126.1;
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.692319631577f ) {
                                            return 226.0/36.0;
                                        } else {
                                            return 176.0/76.0;
                                        }
                                    }
                                } else {
                                    if ( cl->size() <= 34.5f ) {
                                        if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 1.5f ) {
                                                if ( cl->stats.dump_number <= 18.5f ) {
                                                    return 207.0/48.0;
                                                } else {
                                                    return 203.0/90.1;
                                                }
                                            } else {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.38007837534f ) {
                                                    return 280.0/88.0;
                                                } else {
                                                    return 344.0/38.0;
                                                }
                                            }
                                        } else {
                                            return 302.0/32.0;
                                        }
                                    } else {
                                        return 330.0/20.0;
                                    }
                                }
                            }
                        } else {
                            return 271.0/18.0;
                        }
                    }
                }
            } else {
                if ( cl->stats.rdb1_last_touched_diff <= 47877.5f ) {
                    if ( rdb0_last_touched_diff <= 17748.0f ) {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                            return 203.0/428.2;
                        } else {
                            if ( cl->stats.num_overlap_literals <= 7.5f ) {
                                if ( rdb0_last_touched_diff <= 11705.0f ) {
                                    return 25.0/364.2;
                                } else {
                                    return 46.0/304.2;
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.530314564705f ) {
                                    return 59.0/494.3;
                                } else {
                                    return 95.0/374.2;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_queue <= 0.685192465782f ) {
                            if ( cl->stats.dump_number <= 20.5f ) {
                                if ( cl->stats.sum_uip1_used <= 11.5f ) {
                                    if ( cl->stats.num_overlap_literals <= 8.5f ) {
                                        return 204.0/412.2;
                                    } else {
                                        return 229.0/290.2;
                                    }
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 50.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.39786863327f ) {
                                            return 111.0/370.2;
                                        } else {
                                            return 100.0/518.3;
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 33855.5f ) {
                                            return 23.0/434.2;
                                        } else {
                                            return 25.0/362.2;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0786684751511f ) {
                                    return 208.0/330.2;
                                } else {
                                    return 98.0/218.1;
                                }
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.228298604488f ) {
                                if ( cl->stats.sum_uip1_used <= 24.5f ) {
                                    if ( cl->stats.size_rel <= 0.480013489723f ) {
                                        return 135.0/230.1;
                                    } else {
                                        return 178.0/176.1;
                                    }
                                } else {
                                    return 71.0/354.2;
                                }
                            } else {
                                if ( cl->stats.sum_uip1_used <= 8.5f ) {
                                    return 183.0/92.1;
                                } else {
                                    return 126.0/200.1;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.sum_uip1_used <= 20.5f ) {
                        if ( cl->stats.glue <= 7.5f ) {
                            if ( cl->stats.dump_number <= 34.5f ) {
                                if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                    return 161.0/84.0;
                                } else {
                                    if ( cl->size() <= 9.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.075755879283f ) {
                                            return 220.0/176.1;
                                        } else {
                                            if ( cl->stats.sum_uip1_used <= 4.5f ) {
                                                return 138.0/190.1;
                                            } else {
                                                return 169.0/336.2;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.449264109135f ) {
                                            return 132.0/194.1;
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 0.667409062386f ) {
                                                return 143.0/104.1;
                                            } else {
                                                return 176.0/82.0;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_uip1_used <= 9.5f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 270352.0f ) {
                                        return 177.0/92.1;
                                    } else {
                                        return 236.0/54.0;
                                    }
                                } else {
                                    return 267.0/164.1;
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.552801132202f ) {
                                return 298.0/204.1;
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.134737715125f ) {
                                    return 189.0/124.1;
                                } else {
                                    if ( cl->stats.size_rel <= 0.636704981327f ) {
                                        if ( cl->stats.glue_rel_long <= 0.815860629082f ) {
                                            return 253.0/100.1;
                                        } else {
                                            return 153.0/104.1;
                                        }
                                    } else {
                                        if ( cl->stats.glue <= 19.5f ) {
                                            if ( rdb0_act_ranking_top_10 <= 7.5f ) {
                                                if ( cl->stats.rdb1_last_touched_diff <= 109279.0f ) {
                                                    return 182.0/32.0;
                                                } else {
                                                    return 198.0/72.0;
                                                }
                                            } else {
                                                return 218.0/34.0;
                                            }
                                        } else {
                                            return 194.0/88.0;
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.dump_number <= 24.5f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 22.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0385330766439f ) {
                                    return 85.0/272.2;
                                } else {
                                    return 63.0/358.2;
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                    return 176.0/316.2;
                                } else {
                                    return 70.0/286.2;
                                }
                            }
                        } else {
                            if ( cl->stats.glue <= 11.5f ) {
                                if ( cl->stats.glue <= 5.5f ) {
                                    if ( cl->stats.sum_uip1_used <= 109.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0830721110106f ) {
                                            return 192.0/234.1;
                                        } else {
                                            return 207.0/410.2;
                                        }
                                    } else {
                                        if ( cl->size() <= 5.5f ) {
                                            return 68.0/322.2;
                                        } else {
                                            return 102.0/314.2;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.423057675362f ) {
                                        return 143.0/140.1;
                                    } else {
                                        if ( cl->stats.rdb1_act_ranking_top_10 <= 7.5f ) {
                                            if ( cl->stats.num_total_lits_antecedents <= 26.5f ) {
                                                return 96.0/236.1;
                                            } else {
                                                return 155.0/256.1;
                                            }
                                        } else {
                                            return 154.0/202.1;
                                        }
                                    }
                                }
                            } else {
                                return 201.0/168.1;
                            }
                        }
                    }
                }
            }
        }
    } else {
        if ( cl->stats.sum_uip1_used <= 10.5f ) {
            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                if ( cl->stats.num_total_lits_antecedents <= 78.5f ) {
                    if ( cl->stats.dump_number <= 3.5f ) {
                        if ( cl->stats.num_antecedents_rel <= 0.204770386219f ) {
                            return 94.0/200.1;
                        } else {
                            return 124.0/172.1;
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 40.5f ) {
                            return 102.0/200.1;
                        } else {
                            return 238.0/186.1;
                        }
                    }
                } else {
                    if ( cl->stats.size_rel <= 0.492750942707f ) {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 66.0f ) {
                            return 177.0/138.1;
                        } else {
                            return 140.0/258.1;
                        }
                    } else {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 223.0f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 26636.0f ) {
                                if ( cl->stats.num_overlap_literals <= 136.0f ) {
                                    return 202.0/76.0;
                                } else {
                                    return 211.0/24.0;
                                }
                            } else {
                                return 284.0/14.0;
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals <= 201.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 7.64754533768f ) {
                                    return 164.0/90.1;
                                } else {
                                    return 194.0/188.1;
                                }
                            } else {
                                return 183.0/68.0;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.num_overlap_literals <= 19.5f ) {
                    if ( cl->stats.glue_rel_queue <= 0.80788654089f ) {
                        if ( cl->stats.num_antecedents_rel <= 0.129675611854f ) {
                            return 276.0/148.1;
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 6.65499973297f ) {
                                return 201.0/218.1;
                            } else {
                                return 164.0/98.1;
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 39270.0f ) {
                            return 263.0/158.1;
                        } else {
                            if ( cl->stats.num_antecedents_rel <= 0.171309635043f ) {
                                return 380.0/58.0;
                            } else {
                                return 317.0/116.1;
                            }
                        }
                    }
                } else {
                    if ( cl->size() <= 10.5f ) {
                        if ( cl->stats.sum_uip1_used <= 1.5f ) {
                            if ( cl->stats.glue <= 7.5f ) {
                                return 186.0/208.1;
                            } else {
                                return 315.0/66.0;
                            }
                        } else {
                            return 141.0/188.1;
                        }
                    } else {
                        if ( cl->stats.size_rel <= 0.91740834713f ) {
                            if ( rdb0_last_touched_diff <= 60214.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 13.6824598312f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 278.0f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 116.0f ) {
                                            return 217.0/116.1;
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 0.968812346458f ) {
                                                return 169.0/76.0;
                                            } else {
                                                return 260.0/42.0;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.size_rel <= 0.597177267075f ) {
                                            return 157.0/126.1;
                                        } else {
                                            return 160.0/88.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 353.0f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.841525673866f ) {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 5.5f ) {
                                                return 231.0/30.0;
                                            } else {
                                                return 203.0/122.1;
                                            }
                                        } else {
                                            return 216.0/38.0;
                                        }
                                    } else {
                                        if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                            return 208.0/6.0;
                                        } else {
                                            return 272.0/54.0;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.83930760622f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.263168752193f ) {
                                        return 359.0/82.0;
                                    } else {
                                        if ( cl->stats.size_rel <= 0.740264713764f ) {
                                            return 350.0/156.1;
                                        } else {
                                            return 242.0/46.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 197.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.895228743553f ) {
                                            return 219.0/40.0;
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 252362.5f ) {
                                                if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                                    if ( cl->stats.num_total_lits_antecedents <= 112.5f ) {
                                                        return 200.0/24.0;
                                                    } else {
                                                        if ( rdb0_last_touched_diff <= 91970.0f ) {
                                                            return 1;
                                                        } else {
                                                            if ( rdb0_last_touched_diff <= 139318.5f ) {
                                                                return 219.0/16.0;
                                                            } else {
                                                                return 251.0/4.0;
                                                            }
                                                        }
                                                    }
                                                } else {
                                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 7.5f ) {
                                                        if ( cl->stats.num_antecedents_rel <= 0.670690774918f ) {
                                                            return 208.0/24.0;
                                                        } else {
                                                            return 207.0/48.0;
                                                        }
                                                    } else {
                                                        if ( cl->stats.glue_rel_long <= 1.16611266136f ) {
                                                            return 224.0/2.0;
                                                        } else {
                                                            return 197.0/20.0;
                                                        }
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.glue <= 14.5f ) {
                                                    return 209.0/50.0;
                                                } else {
                                                    return 255.0/24.0;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 120.5f ) {
                                            return 224.0/30.0;
                                        } else {
                                            return 284.0/140.1;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.855949163437f ) {
                                if ( cl->stats.size_rel <= 1.32785749435f ) {
                                    if ( cl->stats.dump_number <= 7.5f ) {
                                        return 176.0/58.0;
                                    } else {
                                        return 377.0/30.0;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.762430131435f ) {
                                        return 183.0/102.1;
                                    } else {
                                        return 187.0/32.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 253.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 233.5f ) {
                                        if ( cl->stats.rdb1_act_ranking_top_10 <= 8.5f ) {
                                            if ( cl->size() <= 43.5f ) {
                                                if ( cl->stats.antecedents_glue_long_reds_var <= 28.2161102295f ) {
                                                    if ( cl->size() <= 32.5f ) {
                                                        if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                                            if ( cl->stats.rdb1_last_touched_diff <= 71834.0f ) {
                                                                if ( cl->stats.num_total_lits_antecedents <= 89.5f ) {
                                                                    return 186.0/44.0;
                                                                } else {
                                                                    return 333.0/44.0;
                                                                }
                                                            } else {
                                                                if ( cl->stats.num_overlap_literals_rel <= 0.861496567726f ) {
                                                                    return 189.0/6.0;
                                                                } else {
                                                                    return 243.0/2.0;
                                                                }
                                                            }
                                                        } else {
                                                            return 343.0/78.0;
                                                        }
                                                    } else {
                                                        if ( cl->stats.rdb1_last_touched_diff <= 82197.0f ) {
                                                            return 180.0/50.0;
                                                        } else {
                                                            return 184.0/36.0;
                                                        }
                                                    }
                                                } else {
                                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 4.5f ) {
                                                        return 200.0/16.0;
                                                    } else {
                                                        return 169.0/90.1;
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.glue_rel_long <= 1.44561433792f ) {
                                                    if ( cl->stats.num_antecedents_rel <= 0.248961523175f ) {
                                                        return 204.0/20.0;
                                                    } else {
                                                        return 394.0/12.0;
                                                    }
                                                } else {
                                                    return 195.0/30.0;
                                                }
                                            }
                                        } else {
                                            if ( rdb0_last_touched_diff <= 176153.5f ) {
                                                return 212.0/28.0;
                                            } else {
                                                return 375.0/12.0;
                                            }
                                        }
                                    } else {
                                        return 177.0/78.0;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 2.19448876381f ) {
                                        if ( cl->stats.glue_rel_long <= 1.4584915638f ) {
                                            if ( cl->stats.rdb1_act_ranking_top_10 <= 6.5f ) {
                                                if ( cl->stats.rdb1_last_touched_diff <= 34540.0f ) {
                                                    if ( cl->stats.num_total_lits_antecedents <= 672.5f ) {
                                                        return 259.0/50.0;
                                                    } else {
                                                        return 252.0/12.0;
                                                    }
                                                } else {
                                                    if ( cl->stats.antec_num_total_lits_rel <= 1.602871418f ) {
                                                        if ( rdb0_last_touched_diff <= 106989.0f ) {
                                                            return 262.0/6.0;
                                                        } else {
                                                            return 1;
                                                        }
                                                    } else {
                                                        if ( cl->stats.glue_rel_long <= 1.21903526783f ) {
                                                            return 353.0/22.0;
                                                        } else {
                                                            return 206.0/2.0;
                                                        }
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.glue_rel_long <= 1.12661457062f ) {
                                                    return 284.0/54.0;
                                                } else {
                                                    if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                                        return 189.0/28.0;
                                                    } else {
                                                        return 209.0/12.0;
                                                    }
                                                }
                                            }
                                        } else {
                                            if ( rdb0_last_touched_diff <= 49886.5f ) {
                                                return 183.0/14.0;
                                            } else {
                                                if ( cl->stats.antecedents_glue_long_reds_var <= 20.6065483093f ) {
                                                    return 198.0/10.0;
                                                } else {
                                                    if ( cl->stats.dump_number <= 11.5f ) {
                                                        return 246.0/6.0;
                                                    } else {
                                                        return 1;
                                                    }
                                                }
                                            }
                                        }
                                    } else {
                                        return 205.0/40.0;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                if ( cl->stats.sum_delta_confl_uip1_used <= 4612555.5f ) {
                    if ( cl->stats.used_for_uip_creation <= 5.5f ) {
                        if ( cl->size() <= 37.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 28060.5f ) {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 6.23611116409f ) {
                                        return 95.0/272.2;
                                    } else {
                                        return 72.0/336.2;
                                    }
                                } else {
                                    return 71.0/472.3;
                                }
                            } else {
                                return 136.0/214.1;
                            }
                        } else {
                            return 176.0/386.2;
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 5305.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 739.0f ) {
                                return 23.0/368.2;
                            } else {
                                return 20.0/496.3;
                            }
                        } else {
                            return 66.0/400.2;
                        }
                    }
                } else {
                    if ( cl->stats.sum_uip1_used <= 89.5f ) {
                        return 124.0/520.3;
                    } else {
                        if ( cl->stats.size_rel <= 0.257219970226f ) {
                            return 37.0/386.2;
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.535548627377f ) {
                                return 13.0/476.3;
                            } else {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 4.5f ) {
                                    return 37.0/344.2;
                                } else {
                                    return 22.0/728.4;
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                    if ( cl->stats.sum_uip1_used <= 38.5f ) {
                        if ( cl->stats.glue_rel_queue <= 0.728852033615f ) {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 761818.5f ) {
                                if ( cl->stats.size_rel <= 0.551029920578f ) {
                                    return 87.0/224.1;
                                } else {
                                    return 148.0/160.1;
                                }
                            } else {
                                return 152.0/100.1;
                            }
                        } else {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.404258787632f ) {
                                if ( cl->stats.glue_rel_long <= 0.884648442268f ) {
                                    return 167.0/84.0;
                                } else {
                                    return 183.0/60.0;
                                }
                            } else {
                                if ( cl->size() <= 36.5f ) {
                                    return 131.0/190.1;
                                } else {
                                    return 183.0/86.0;
                                }
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 75803.0f ) {
                            if ( cl->stats.num_antecedents_rel <= 0.21874460578f ) {
                                return 89.0/284.2;
                            } else {
                                return 56.0/302.2;
                            }
                        } else {
                            return 193.0/388.2;
                        }
                    }
                } else {
                    return 93.0/488.3;
                }
            }
        }
    }
}

static bool should_keep_long_conf2_cluster0(
    const CMSat::Clause* cl
    , const uint64_t sumConflicts
    , const uint32_t rdb0_last_touched_diff
    , const uint32_t rdb0_act_ranking
    , const uint32_t rdb0_act_ranking_top_10
) {
    int votes = 0;
    votes += estimator_should_keep_long_conf2_cluster0_0(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_long_conf2_cluster0_1(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_long_conf2_cluster0_2(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_long_conf2_cluster0_3(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_long_conf2_cluster0_4(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_long_conf2_cluster0_5(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_long_conf2_cluster0_6(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_long_conf2_cluster0_7(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_long_conf2_cluster0_8(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_long_conf2_cluster0_9(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    return votes >= 5;
}
}
