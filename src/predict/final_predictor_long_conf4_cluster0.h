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

static double estimator_should_keep_long_conf4_cluster0_0(
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
        if ( cl->stats.antecedents_glue_long_reds_var <= 2.28782248497f ) {
            if ( cl->stats.size_rel <= 0.476849913597f ) {
                if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                    if ( cl->stats.num_overlap_literals <= 16.5f ) {
                        if ( rdb0_last_touched_diff <= 44668.5f ) {
                            if ( cl->stats.sum_uip1_used <= 17.5f ) {
                                if ( cl->stats.dump_number <= 17.5f ) {
                                    if ( cl->size() <= 10.5f ) {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 10.5f ) {
                                            return 162.0/414.0;
                                        } else {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 4.5f ) {
                                                if ( cl->stats.size_rel <= 0.281631529331f ) {
                                                    if ( cl->stats.num_antecedents_rel <= 0.117552176118f ) {
                                                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                                            return 64.0/412.0;
                                                        } else {
                                                            return 93.0/378.0;
                                                        }
                                                    } else {
                                                        return 155.0/456.0;
                                                    }
                                                } else {
                                                    if ( cl->stats.antec_num_total_lits_rel <= 0.144060134888f ) {
                                                        return 65.0/324.0;
                                                    } else {
                                                        return 54.0/392.0;
                                                    }
                                                }
                                            } else {
                                                return 52.0/420.0;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0178163331002f ) {
                                            return 105.0/206.0;
                                        } else {
                                            return 169.0/440.0;
                                        }
                                    }
                                } else {
                                    return 185.0/376.0;
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 6130.0f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 5.5f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 17.5f ) {
                                            if ( cl->stats.glue_rel_queue <= 0.412456184626f ) {
                                                return 45.0/638.0;
                                            } else {
                                                return 61.0/430.0;
                                            }
                                        } else {
                                            return 84.0/550.0;
                                        }
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.093573525548f ) {
                                            return 49.0/693.9;
                                        } else {
                                            return 15.0/540.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.102134428918f ) {
                                        if ( cl->stats.num_overlap_literals <= 5.5f ) {
                                            if ( cl->stats.num_antecedents_rel <= 0.0443728752434f ) {
                                                return 70.0/302.0;
                                            } else {
                                                if ( cl->stats.rdb1_last_touched_diff <= 16628.5f ) {
                                                    return 35.0/392.0;
                                                } else {
                                                    return 46.0/282.0;
                                                }
                                            }
                                        } else {
                                            return 127.0/572.0;
                                        }
                                    } else {
                                        if ( cl->stats.sum_uip1_used <= 40.5f ) {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 1748077.5f ) {
                                                return 48.0/342.0;
                                            } else {
                                                return 66.0/286.0;
                                            }
                                        } else {
                                            if ( cl->stats.num_overlap_literals <= 4.5f ) {
                                                return 64.0/562.0;
                                            } else {
                                                if ( cl->stats.num_overlap_literals_rel <= 0.0605124458671f ) {
                                                    return 46.0/344.0;
                                                } else {
                                                    if ( cl->stats.num_overlap_literals <= 8.5f ) {
                                                        return 22.0/498.0;
                                                    } else {
                                                        return 39.0/410.0;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 640656.0f ) {
                                if ( cl->stats.glue_rel_queue <= 0.392545282841f ) {
                                    return 131.0/300.0;
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0958812236786f ) {
                                        return 135.0/262.0;
                                    } else {
                                        return 130.0/180.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_uip1_used <= 89.5f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0269742496312f ) {
                                        return 73.0/258.0;
                                    } else {
                                        return 151.0/356.0;
                                    }
                                } else {
                                    return 102.0/494.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 794.5f ) {
                            if ( cl->stats.glue_rel_long <= 0.718933343887f ) {
                                return 183.0/312.0;
                            } else {
                                return 161.0/132.0;
                            }
                        } else {
                            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.glue_rel_long <= 0.350051045418f ) {
                                    return 100.0/222.0;
                                } else {
                                    if ( cl->stats.size_rel <= 0.250874340534f ) {
                                        return 87.0/506.0;
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.597269058228f ) {
                                            return 77.0/266.0;
                                        } else {
                                            return 60.0/316.0;
                                        }
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 59270.5f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                        if ( cl->stats.sum_uip1_used <= 29.5f ) {
                                            if ( rdb0_last_touched_diff <= 27907.5f ) {
                                                return 109.0/260.0;
                                            } else {
                                                return 117.0/178.0;
                                            }
                                        } else {
                                            return 71.0/312.0;
                                        }
                                    } else {
                                        return 77.0/432.0;
                                    }
                                } else {
                                    return 138.0/170.0;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.sum_uip1_used <= 29.5f ) {
                        if ( cl->stats.sum_uip1_used <= 13.5f ) {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                                if ( cl->size() <= 8.5f ) {
                                    return 94.0/560.0;
                                } else {
                                    return 119.0/342.0;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 11.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.152099370956f ) {
                                        if ( cl->stats.num_overlap_literals <= 4.5f ) {
                                            return 43.0/428.0;
                                        } else {
                                            return 59.0/442.0;
                                        }
                                    } else {
                                        return 19.0/388.0;
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 0.249234080315f ) {
                                        return 59.0/352.0;
                                    } else {
                                        return 38.0/332.0;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.248966932297f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 11.5f ) {
                                    return 53.0/536.0;
                                } else {
                                    if ( cl->stats.dump_number <= 11.5f ) {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 262671.5f ) {
                                            return 22.0/679.9;
                                        } else {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0415185242891f ) {
                                                return 32.0/336.0;
                                            } else {
                                                return 20.0/476.0;
                                            }
                                        }
                                    } else {
                                        return 59.0/430.0;
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 3511.0f ) {
                                    return 31.0/408.0;
                                } else {
                                    return 63.0/324.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 9928.5f ) {
                            if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 1126.0f ) {
                                    if ( cl->stats.glue_rel_long <= 0.290842324495f ) {
                                        return 29.0/382.0;
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.105088219047f ) {
                                            return 6.0/618.0;
                                        } else {
                                            return 17.0/432.0;
                                        }
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 7668.0f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.06060025841f ) {
                                            if ( cl->stats.num_total_lits_antecedents <= 16.5f ) {
                                                return 55.0/516.0;
                                            } else {
                                                return 23.0/406.0;
                                            }
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 22.5f ) {
                                                if ( cl->stats.sum_delta_confl_uip1_used <= 25689784.0f ) {
                                                    if ( cl->stats.num_overlap_literals <= 4.5f ) {
                                                        return 6.0/410.0;
                                                    } else {
                                                        return 8.0/396.0;
                                                    }
                                                } else {
                                                    return 15.0/374.0;
                                                }
                                            } else {
                                                if ( cl->size() <= 12.5f ) {
                                                    return 29.0/404.0;
                                                } else {
                                                    return 13.0/414.0;
                                                }
                                            }
                                        }
                                    } else {
                                        return 54.0/530.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 414205.5f ) {
                                    return 30.0/534.0;
                                } else {
                                    if ( cl->stats.dump_number <= 11.5f ) {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 755485.0f ) {
                                            return 18.0/536.0;
                                        } else {
                                            if ( cl->stats.used_for_uip_creation <= 9.5f ) {
                                                if ( cl->stats.dump_number <= 7.5f ) {
                                                    return 9.0/608.0;
                                                } else {
                                                    return 15.0/396.0;
                                                }
                                            } else {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.0699879899621f ) {
                                                    if ( cl->stats.sum_uip1_used <= 190.5f ) {
                                                        if ( cl->stats.glue_rel_long <= 0.315903365612f ) {
                                                            return 6.0/490.0;
                                                        } else {
                                                            return 18.0/632.0;
                                                        }
                                                    } else {
                                                        return 10.0/837.9;
                                                    }
                                                } else {
                                                    if ( cl->size() <= 17.5f ) {
                                                        if ( cl->stats.sum_uip1_used <= 109.5f ) {
                                                            return 4.0/630.0;
                                                        } else {
                                                            return 1.0/606.0;
                                                        }
                                                    } else {
                                                        return 5.0/396.0;
                                                    }
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 6925.5f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0335082300007f ) {
                                                if ( cl->stats.sum_delta_confl_uip1_used <= 31590724.0f ) {
                                                    if ( cl->stats.sum_uip1_used <= 150.5f ) {
                                                        return 17.0/454.0;
                                                    } else {
                                                        return 3.0/466.0;
                                                    }
                                                } else {
                                                    if ( cl->stats.dump_number <= 22.5f ) {
                                                        return 19.0/404.0;
                                                    } else {
                                                        if ( cl->stats.rdb1_used_for_uip_creation <= 12.5f ) {
                                                            return 23.0/374.0;
                                                        } else {
                                                            return 14.0/699.9;
                                                        }
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.sum_uip1_used <= 105.5f ) {
                                                    return 20.0/482.0;
                                                } else {
                                                    if ( rdb0_last_touched_diff <= 2131.5f ) {
                                                        if ( cl->stats.num_overlap_literals <= 9.5f ) {
                                                            if ( cl->stats.glue_rel_queue <= 0.365297108889f ) {
                                                                return 0.0/476.0;
                                                            } else {
                                                                return 3.0/530.0;
                                                            }
                                                        } else {
                                                            return 16.0/638.0;
                                                        }
                                                    } else {
                                                        if ( cl->stats.glue_rel_long <= 0.403982400894f ) {
                                                            return 16.0/418.0;
                                                        } else {
                                                            return 5.0/424.0;
                                                        }
                                                    }
                                                }
                                            }
                                        } else {
                                            return 21.0/440.0;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.234028398991f ) {
                                return 47.0/336.0;
                            } else {
                                if ( rdb0_last_touched_diff <= 793.0f ) {
                                    return 24.0/530.0;
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 124.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0428293570876f ) {
                                            return 38.0/456.0;
                                        } else {
                                            return 83.0/594.0;
                                        }
                                    } else {
                                        return 31.0/783.9;
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                    if ( cl->stats.sum_uip1_used <= 17.5f ) {
                        if ( cl->stats.num_overlap_literals <= 20.5f ) {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.213359728456f ) {
                                if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                                    return 163.0/400.0;
                                } else {
                                    return 65.0/476.0;
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.673158288002f ) {
                                    return 87.0/372.0;
                                } else {
                                    return 186.0/306.0;
                                }
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 2.5f ) {
                                return 222.0/148.0;
                            } else {
                                if ( cl->stats.dump_number <= 7.5f ) {
                                    return 152.0/462.0;
                                } else {
                                    return 162.0/220.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                            if ( cl->stats.sum_uip1_used <= 38.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.110766761005f ) {
                                    return 75.0/402.0;
                                } else {
                                    return 101.0/430.0;
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 16661.5f ) {
                                    if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                        return 66.0/640.0;
                                    } else {
                                        return 19.0/426.0;
                                    }
                                } else {
                                    return 79.0/472.0;
                                }
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 98.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 90.5f ) {
                                    if ( cl->stats.dump_number <= 12.5f ) {
                                        return 24.0/699.9;
                                    } else {
                                        return 47.0/442.0;
                                    }
                                } else {
                                    return 46.0/330.0;
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.718602061272f ) {
                                    if ( cl->stats.num_overlap_literals <= 11.5f ) {
                                        return 8.0/384.0;
                                    } else {
                                        return 21.0/404.0;
                                    }
                                } else {
                                    return 9.0/701.9;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                        if ( cl->stats.num_antecedents_rel <= 0.495378375053f ) {
                            if ( cl->stats.glue_rel_long <= 0.773765742779f ) {
                                if ( cl->stats.sum_uip1_used <= 7.5f ) {
                                    if ( cl->stats.dump_number <= 7.5f ) {
                                        if ( cl->size() <= 15.5f ) {
                                            return 133.0/218.0;
                                        } else {
                                            return 137.0/176.0;
                                        }
                                    } else {
                                        return 163.0/138.0;
                                    }
                                } else {
                                    if ( cl->stats.glue <= 7.5f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 24.5f ) {
                                            return 59.0/400.0;
                                        } else {
                                            return 134.0/438.0;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.596471071243f ) {
                                            return 99.0/186.0;
                                        } else {
                                            return 106.0/352.0;
                                        }
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 25852.5f ) {
                                    if ( cl->stats.dump_number <= 3.5f ) {
                                        return 149.0/158.0;
                                    } else {
                                        return 124.0/350.0;
                                    }
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.195790827274f ) {
                                        return 161.0/222.0;
                                    } else {
                                        return 232.0/206.0;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.62451851368f ) {
                                return 126.0/182.0;
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 99.0f ) {
                                    return 215.0/58.0;
                                } else {
                                    return 187.0/234.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 4175.5f ) {
                                return 60.0/368.0;
                            } else {
                                return 99.0/264.0;
                            }
                        } else {
                            return 46.0/462.0;
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.sum_uip1_used <= 4.5f ) {
                if ( cl->stats.glue_rel_long <= 0.981121778488f ) {
                    if ( cl->stats.sum_uip1_used <= 1.5f ) {
                        if ( cl->stats.dump_number <= 2.5f ) {
                            if ( cl->stats.size_rel <= 0.555395722389f ) {
                                return 186.0/184.0;
                            } else {
                                return 306.0/144.0;
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals <= 142.5f ) {
                                if ( cl->stats.glue_rel_long <= 0.823705017567f ) {
                                    return 217.0/140.0;
                                } else {
                                    return 162.0/58.0;
                                }
                            } else {
                                return 213.0/64.0;
                            }
                        }
                    } else {
                        if ( cl->stats.size_rel <= 0.606517016888f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 13174.5f ) {
                                return 102.0/348.0;
                            } else {
                                return 189.0/224.0;
                            }
                        } else {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                return 352.0/422.0;
                            } else {
                                return 227.0/138.0;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.sum_uip1_used <= 1.5f ) {
                        if ( cl->stats.num_overlap_literals <= 99.5f ) {
                            if ( cl->stats.glue <= 13.5f ) {
                                return 301.0/146.0;
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.539886653423f ) {
                                    return 187.0/52.0;
                                } else {
                                    return 199.0/18.0;
                                }
                            }
                        } else {
                            if ( cl->stats.num_antecedents_rel <= 2.22370553017f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 726.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 1.48905181885f ) {
                                        if ( cl->stats.size_rel <= 0.800171375275f ) {
                                            return 179.0/24.0;
                                        } else {
                                            return 365.0/10.0;
                                        }
                                    } else {
                                        return 244.0/38.0;
                                    }
                                } else {
                                    return 260.0/2.0;
                                }
                            } else {
                                return 282.0/42.0;
                            }
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 11.5941047668f ) {
                            return 241.0/186.0;
                        } else {
                            if ( cl->stats.num_overlap_literals_rel <= 0.689036607742f ) {
                                return 170.0/66.0;
                            } else {
                                return 255.0/56.0;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.sum_delta_confl_uip1_used <= 1605239.5f ) {
                    if ( rdb0_last_touched_diff <= 37454.5f ) {
                        if ( cl->stats.size_rel <= 1.0733935833f ) {
                            if ( cl->stats.sum_uip1_used <= 13.5f ) {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                    if ( cl->stats.dump_number <= 10.5f ) {
                                        if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                                            return 155.0/408.0;
                                        } else {
                                            return 68.0/366.0;
                                        }
                                    } else {
                                        return 122.0/250.0;
                                    }
                                } else {
                                    return 120.0/536.0;
                                }
                            } else {
                                if ( cl->stats.dump_number <= 9.5f ) {
                                    if ( cl->stats.num_overlap_literals <= 64.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.615773379803f ) {
                                            return 33.0/408.0;
                                        } else {
                                            return 80.0/594.0;
                                        }
                                    } else {
                                        return 67.0/338.0;
                                    }
                                } else {
                                    return 82.0/266.0;
                                }
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 179.5f ) {
                                return 89.0/298.0;
                            } else {
                                return 158.0/280.0;
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_queue <= 0.82769048214f ) {
                            if ( cl->stats.dump_number <= 10.5f ) {
                                return 105.0/282.0;
                            } else {
                                return 153.0/188.0;
                            }
                        } else {
                            return 237.0/206.0;
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_last_touched_diff <= 19245.5f ) {
                        if ( rdb0_last_touched_diff <= 4617.0f ) {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                return 33.0/406.0;
                            } else {
                                if ( cl->stats.size_rel <= 0.500609397888f ) {
                                    return 7.0/440.0;
                                } else {
                                    return 19.0/416.0;
                                }
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals_rel <= 0.380825698376f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 10003028.0f ) {
                                    return 104.0/472.0;
                                } else {
                                    return 38.0/350.0;
                                }
                            } else {
                                return 45.0/388.0;
                            }
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 5.2109375f ) {
                            return 69.0/326.0;
                        } else {
                            return 143.0/368.0;
                        }
                    }
                }
            }
        }
    } else {
        if ( cl->stats.sum_delta_confl_uip1_used <= 1636.0f ) {
            if ( cl->stats.num_overlap_literals <= 31.5f ) {
                if ( rdb0_last_touched_diff <= 88226.0f ) {
                    if ( cl->stats.glue_rel_queue <= 0.879013538361f ) {
                        if ( rdb0_last_touched_diff <= 61216.5f ) {
                            if ( cl->stats.num_overlap_literals <= 6.5f ) {
                                return 156.0/448.0;
                            } else {
                                if ( cl->size() <= 15.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.61610352993f ) {
                                        return 160.0/204.0;
                                    } else {
                                        return 115.0/230.0;
                                    }
                                } else {
                                    return 204.0/176.0;
                                }
                            }
                        } else {
                            if ( cl->stats.size_rel <= 0.400808602571f ) {
                                return 153.0/196.0;
                            } else {
                                return 240.0/162.0;
                            }
                        }
                    } else {
                        if ( cl->stats.glue <= 8.5f ) {
                            return 161.0/164.0;
                        } else {
                            if ( cl->stats.glue_rel_queue <= 1.04865789413f ) {
                                return 223.0/144.0;
                            } else {
                                if ( cl->size() <= 34.0f ) {
                                    return 226.0/62.0;
                                } else {
                                    return 156.0/74.0;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.size_rel <= 0.644912242889f ) {
                        if ( rdb0_last_touched_diff <= 298472.5f ) {
                            if ( cl->stats.num_antecedents_rel <= 0.0551249906421f ) {
                                return 181.0/74.0;
                            } else {
                                if ( cl->size() <= 12.5f ) {
                                    if ( cl->size() <= 6.5f ) {
                                        return 142.0/188.0;
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.113318733871f ) {
                                            return 213.0/222.0;
                                        } else {
                                            return 238.0/152.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.775092542171f ) {
                                        return 254.0/148.0;
                                    } else {
                                        return 172.0/50.0;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.size_rel <= 0.288753569126f ) {
                                return 164.0/94.0;
                            } else {
                                return 308.0/72.0;
                            }
                        }
                    } else {
                        if ( cl->stats.dump_number <= 23.5f ) {
                            if ( cl->stats.glue_rel_queue <= 0.977852225304f ) {
                                if ( cl->stats.size_rel <= 1.04270887375f ) {
                                    if ( cl->size() <= 17.5f ) {
                                        return 225.0/48.0;
                                    } else {
                                        return 205.0/78.0;
                                    }
                                } else {
                                    return 212.0/114.0;
                                }
                            } else {
                                if ( cl->stats.size_rel <= 1.19048750401f ) {
                                    return 303.0/40.0;
                                } else {
                                    return 204.0/48.0;
                                }
                            }
                        } else {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.395835101604f ) {
                                if ( cl->stats.glue_rel_long <= 0.803358197212f ) {
                                    return 192.0/58.0;
                                } else {
                                    return 257.0/32.0;
                                }
                            } else {
                                if ( cl->size() <= 21.5f ) {
                                    return 215.0/14.0;
                                } else {
                                    return 211.0/2.0;
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.sum_delta_confl_uip1_used <= 67.5f ) {
                    if ( cl->stats.glue <= 8.5f ) {
                        if ( cl->stats.size_rel <= 0.202026963234f ) {
                            return 143.0/148.0;
                        } else {
                            if ( cl->stats.num_overlap_literals_rel <= 0.358349740505f ) {
                                return 262.0/40.0;
                            } else {
                                if ( cl->size() <= 13.5f ) {
                                    return 262.0/180.0;
                                } else {
                                    if ( rdb0_last_touched_diff <= 90926.5f ) {
                                        return 174.0/92.0;
                                    } else {
                                        return 338.0/98.0;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_queue <= 0.916581809521f ) {
                            if ( cl->stats.glue <= 15.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.62716782093f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.358866453171f ) {
                                        return 222.0/48.0;
                                    } else {
                                        return 287.0/38.0;
                                    }
                                } else {
                                    if ( cl->size() <= 29.5f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 134.5f ) {
                                            return 178.0/46.0;
                                        } else {
                                            return 183.0/94.0;
                                        }
                                    } else {
                                        return 202.0/30.0;
                                    }
                                }
                            } else {
                                if ( cl->size() <= 56.5f ) {
                                    return 189.0/104.0;
                                } else {
                                    return 231.0/56.0;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 8.5f ) {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 6.5f ) {
                                    if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                        if ( cl->stats.num_overlap_literals <= 43.5f ) {
                                            return 322.0/12.0;
                                        } else {
                                            if ( cl->stats.dump_number <= 7.5f ) {
                                                if ( cl->stats.glue_rel_long <= 1.22493386269f ) {
                                                    if ( cl->stats.glue_rel_queue <= 1.06268155575f ) {
                                                        return 355.0/42.0;
                                                    } else {
                                                        if ( cl->stats.glue_rel_queue <= 1.13994145393f ) {
                                                            return 194.0/64.0;
                                                        } else {
                                                            return 223.0/32.0;
                                                        }
                                                    }
                                                } else {
                                                    if ( cl->stats.num_antecedents_rel <= 0.49413228035f ) {
                                                        return 204.0/36.0;
                                                    } else {
                                                        if ( cl->stats.antec_num_total_lits_rel <= 1.6539516449f ) {
                                                            return 262.0/16.0;
                                                        } else {
                                                            return 239.0/6.0;
                                                        }
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.glue_rel_queue <= 1.01246452332f ) {
                                                    return 405.0/40.0;
                                                } else {
                                                    if ( cl->stats.rdb1_last_touched_diff <= 195505.5f ) {
                                                        if ( cl->stats.glue_rel_queue <= 1.3909034729f ) {
                                                            if ( cl->size() <= 25.5f ) {
                                                                return 260.0/6.0;
                                                            } else {
                                                                if ( cl->size() <= 59.5f ) {
                                                                    return 220.0/30.0;
                                                                } else {
                                                                    return 276.0/8.0;
                                                                }
                                                            }
                                                        } else {
                                                            return 186.0/28.0;
                                                        }
                                                    } else {
                                                        if ( cl->stats.glue_rel_long <= 1.26385128498f ) {
                                                            return 325.0/2.0;
                                                        } else {
                                                            return 228.0/10.0;
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 1.10257279873f ) {
                                            if ( rdb0_last_touched_diff <= 108997.5f ) {
                                                return 278.0/104.0;
                                            } else {
                                                return 262.0/30.0;
                                            }
                                        } else {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 18.0381946564f ) {
                                                if ( cl->stats.num_antecedents_rel <= 0.887643992901f ) {
                                                    return 194.0/52.0;
                                                } else {
                                                    return 268.0/28.0;
                                                }
                                            } else {
                                                if ( cl->stats.sum_delta_confl_uip1_used <= 1.5f ) {
                                                    return 235.0/30.0;
                                                } else {
                                                    return 328.0/8.0;
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.54870057106f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.679747283459f ) {
                                            return 365.0/82.0;
                                        } else {
                                            return 166.0/74.0;
                                        }
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 676.5f ) {
                                            if ( cl->stats.size_rel <= 0.951156139374f ) {
                                                return 214.0/54.0;
                                            } else {
                                                if ( cl->stats.antecedents_glue_long_reds_var <= 9.59309387207f ) {
                                                    return 263.0/38.0;
                                                } else {
                                                    if ( cl->stats.glue_rel_queue <= 1.3036096096f ) {
                                                        return 263.0/8.0;
                                                    } else {
                                                        return 207.0/12.0;
                                                    }
                                                }
                                            }
                                        } else {
                                            return 249.0/56.0;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 1.23514556885f ) {
                                    if ( rdb0_last_touched_diff <= 138765.0f ) {
                                        return 290.0/20.0;
                                    } else {
                                        if ( cl->stats.size_rel <= 1.07856810093f ) {
                                            return 329.0/22.0;
                                        } else {
                                            return 1;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 1.71044826508f ) {
                                        return 196.0/32.0;
                                    } else {
                                        return 405.0/16.0;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.glue <= 8.5f ) {
                        return 144.0/160.0;
                    } else {
                        if ( rdb0_last_touched_diff <= 161245.5f ) {
                            if ( cl->stats.glue_rel_queue <= 0.929082632065f ) {
                                return 194.0/100.0;
                            } else {
                                if ( cl->stats.glue_rel_long <= 1.16334152222f ) {
                                    return 234.0/30.0;
                                } else {
                                    return 191.0/66.0;
                                }
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals_rel <= 0.68522387743f ) {
                                return 212.0/22.0;
                            } else {
                                return 243.0/56.0;
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.glue_rel_queue <= 0.701444029808f ) {
                if ( rdb0_last_touched_diff <= 70957.0f ) {
                    if ( cl->stats.num_overlap_literals <= 10.5f ) {
                        if ( rdb0_last_touched_diff <= 5674.5f ) {
                            if ( cl->stats.glue_rel_queue <= 0.562008261681f ) {
                                if ( cl->stats.dump_number <= 23.5f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.026555467397f ) {
                                        return 29.0/366.0;
                                    } else {
                                        return 34.0/667.9;
                                    }
                                } else {
                                    return 60.0/284.0;
                                }
                            } else {
                                return 43.0/308.0;
                            }
                        } else {
                            if ( cl->stats.dump_number <= 26.5f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 418685.0f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.0615891814232f ) {
                                        return 108.0/248.0;
                                    } else {
                                        if ( cl->stats.sum_uip1_used <= 5.5f ) {
                                            return 120.0/314.0;
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.152146458626f ) {
                                                return 84.0/318.0;
                                            } else {
                                                return 47.0/368.0;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 0.150209575891f ) {
                                        return 30.0/360.0;
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.14546662569f ) {
                                            return 67.0/298.0;
                                        } else {
                                            return 58.0/382.0;
                                        }
                                    }
                                }
                            } else {
                                return 144.0/364.0;
                            }
                        }
                    } else {
                        if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                            if ( cl->stats.dump_number <= 5.5f ) {
                                if ( cl->stats.num_overlap_literals <= 28.5f ) {
                                    return 83.0/286.0;
                                } else {
                                    return 56.0/318.0;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 43.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.548902750015f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.192535862327f ) {
                                            return 123.0/268.0;
                                        } else {
                                            return 71.0/386.0;
                                        }
                                    } else {
                                        return 163.0/322.0;
                                    }
                                } else {
                                    return 183.0/290.0;
                                }
                            }
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 7.5f ) {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                                    return 47.0/360.0;
                                } else {
                                    return 89.0/352.0;
                                }
                            } else {
                                return 14.0/410.0;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_overlap_literals_rel <= 0.262737333775f ) {
                        if ( cl->size() <= 8.5f ) {
                            if ( rdb0_last_touched_diff <= 198536.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 123420.5f ) {
                                    if ( cl->stats.sum_uip1_used <= 39.5f ) {
                                        if ( cl->stats.glue_rel_long <= 0.341374367476f ) {
                                            return 144.0/158.0;
                                        } else {
                                            return 168.0/302.0;
                                        }
                                    } else {
                                        return 99.0/426.0;
                                    }
                                } else {
                                    if ( cl->size() <= 5.5f ) {
                                        return 74.0/342.0;
                                    } else {
                                        return 147.0/300.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_uip1_used <= 30.5f ) {
                                    if ( rdb0_last_touched_diff <= 296234.5f ) {
                                        return 141.0/174.0;
                                    } else {
                                        return 179.0/146.0;
                                    }
                                } else {
                                    return 179.0/350.0;
                                }
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 9.5f ) {
                                if ( cl->stats.dump_number <= 20.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 36.5f ) {
                                        return 140.0/172.0;
                                    } else {
                                        return 118.0/176.0;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0917757600546f ) {
                                        return 263.0/182.0;
                                    } else {
                                        return 300.0/148.0;
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 165127.0f ) {
                                    if ( cl->stats.dump_number <= 25.5f ) {
                                        if ( cl->stats.size_rel <= 0.45118406415f ) {
                                            return 133.0/354.0;
                                        } else {
                                            return 80.0/284.0;
                                        }
                                    } else {
                                        return 218.0/362.0;
                                    }
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 59.5f ) {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 1243841.5f ) {
                                            return 267.0/228.0;
                                        } else {
                                            return 169.0/190.0;
                                        }
                                    } else {
                                        return 158.0/268.0;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.sum_uip1_used <= 11.5f ) {
                            if ( cl->stats.dump_number <= 28.5f ) {
                                return 206.0/184.0;
                            } else {
                                return 195.0/72.0;
                            }
                        } else {
                            if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                return 143.0/170.0;
                            } else {
                                return 109.0/216.0;
                            }
                        }
                    }
                }
            } else {
                if ( rdb0_last_touched_diff <= 46008.0f ) {
                    if ( cl->stats.antec_num_total_lits_rel <= 0.690255761147f ) {
                        if ( cl->stats.sum_uip1_used <= 19.5f ) {
                            if ( cl->stats.dump_number <= 11.5f ) {
                                if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 4.5f ) {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.361111104488f ) {
                                            return 100.0/260.0;
                                        } else {
                                            return 176.0/232.0;
                                        }
                                    } else {
                                        return 104.0/276.0;
                                    }
                                } else {
                                    return 72.0/420.0;
                                }
                            } else {
                                return 252.0/348.0;
                            }
                        } else {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.322908401489f ) {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                                    return 60.0/334.0;
                                } else {
                                    return 52.0/356.0;
                                }
                            } else {
                                return 36.0/380.0;
                            }
                        }
                    } else {
                        if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                            return 208.0/232.0;
                        } else {
                            return 101.0/212.0;
                        }
                    }
                } else {
                    if ( cl->stats.antecedents_glue_long_reds_var <= 9.57943058014f ) {
                        if ( cl->stats.num_overlap_literals_rel <= 0.467727303505f ) {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 7664577.0f ) {
                                if ( cl->stats.dump_number <= 35.5f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 1.70089292526f ) {
                                        if ( cl->size() <= 56.5f ) {
                                            if ( cl->size() <= 11.5f ) {
                                                if ( cl->stats.rdb1_last_touched_diff <= 92521.5f ) {
                                                    return 94.0/232.0;
                                                } else {
                                                    return 119.0/170.0;
                                                }
                                            } else {
                                                if ( cl->stats.size_rel <= 0.768107533455f ) {
                                                    if ( cl->stats.num_antecedents_rel <= 0.17237919569f ) {
                                                        return 137.0/132.0;
                                                    } else {
                                                        return 132.0/220.0;
                                                    }
                                                } else {
                                                    return 202.0/112.0;
                                                }
                                            }
                                        } else {
                                            return 124.0/324.0;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.86021900177f ) {
                                            return 185.0/214.0;
                                        } else {
                                            return 247.0/162.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 17.5f ) {
                                        if ( cl->stats.sum_uip1_used <= 3.5f ) {
                                            return 178.0/54.0;
                                        } else {
                                            if ( cl->size() <= 22.5f ) {
                                                return 265.0/168.0;
                                            } else {
                                                return 187.0/68.0;
                                            }
                                        }
                                    } else {
                                        return 172.0/156.0;
                                    }
                                }
                            } else {
                                return 162.0/388.0;
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 204236.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 1.16463446617f ) {
                                    if ( cl->stats.size_rel <= 0.893313169479f ) {
                                        return 185.0/204.0;
                                    } else {
                                        return 198.0/132.0;
                                    }
                                } else {
                                    return 139.0/172.0;
                                }
                            } else {
                                return 329.0/96.0;
                            }
                        }
                    } else {
                        if ( cl->stats.sum_uip1_used <= 8.5f ) {
                            if ( cl->size() <= 22.5f ) {
                                return 163.0/82.0;
                            } else {
                                if ( cl->stats.sum_uip1_used <= 4.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 1.05114924908f ) {
                                        return 191.0/50.0;
                                    } else {
                                        return 221.0/24.0;
                                    }
                                } else {
                                    return 186.0/80.0;
                                }
                            }
                        } else {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.515518903732f ) {
                                return 134.0/154.0;
                            } else {
                                return 150.0/120.0;
                            }
                        }
                    }
                }
            }
        }
    }
}

static double estimator_should_keep_long_conf4_cluster0_1(
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
        if ( cl->stats.sum_uip1_used <= 3.5f ) {
            if ( cl->stats.size_rel <= 0.570319950581f ) {
                if ( cl->stats.rdb1_last_touched_diff <= 31746.5f ) {
                    if ( cl->stats.num_antecedents_rel <= 0.335621446371f ) {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 9.46999931335f ) {
                            if ( cl->stats.num_overlap_literals <= 12.5f ) {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.31786569953f ) {
                                        return 79.0/306.0;
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0196493752301f ) {
                                            return 133.0/214.0;
                                        } else {
                                            if ( cl->size() <= 8.5f ) {
                                                return 150.0/432.0;
                                            } else {
                                                return 215.0/426.0;
                                            }
                                        }
                                    }
                                } else {
                                    return 69.0/290.0;
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                                    if ( cl->stats.glue <= 6.5f ) {
                                        return 92.0/254.0;
                                    } else {
                                        return 127.0/154.0;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.612282931805f ) {
                                        return 106.0/186.0;
                                    } else {
                                        return 234.0/162.0;
                                    }
                                }
                            }
                        } else {
                            return 191.0/90.0;
                        }
                    } else {
                        if ( cl->stats.num_overlap_literals_rel <= 0.525378763676f ) {
                            if ( cl->stats.glue_rel_long <= 0.803714990616f ) {
                                return 190.0/360.0;
                            } else {
                                return 144.0/110.0;
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 8.07843780518f ) {
                                if ( rdb0_last_touched_diff <= 22446.0f ) {
                                    return 106.0/204.0;
                                } else {
                                    return 158.0/146.0;
                                }
                            } else {
                                return 347.0/66.0;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.glue_rel_queue <= 0.840825557709f ) {
                        if ( cl->stats.glue <= 8.5f ) {
                            if ( cl->size() <= 8.5f ) {
                                if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 26.5f ) {
                                        return 182.0/342.0;
                                    } else {
                                        return 96.0/258.0;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.355345010757f ) {
                                        return 210.0/280.0;
                                    } else {
                                        if ( cl->stats.dump_number <= 9.5f ) {
                                            return 117.0/164.0;
                                        } else {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.122450180352f ) {
                                                return 161.0/152.0;
                                            } else {
                                                return 171.0/90.0;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                                    return 142.0/194.0;
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.36880427599f ) {
                                        return 180.0/172.0;
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 76623.5f ) {
                                            return 190.0/190.0;
                                        } else {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.215823844075f ) {
                                                if ( cl->stats.glue_rel_queue <= 0.60313719511f ) {
                                                    return 249.0/60.0;
                                                } else {
                                                    return 226.0/86.0;
                                                }
                                            } else {
                                                return 148.0/136.0;
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.79193842411f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.0546875f ) {
                                    return 200.0/180.0;
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.64266204834f ) {
                                        return 183.0/60.0;
                                    } else {
                                        return 158.0/110.0;
                                    }
                                }
                            } else {
                                return 187.0/54.0;
                            }
                        }
                    } else {
                        if ( cl->stats.antec_num_total_lits_rel <= 0.828485488892f ) {
                            if ( cl->stats.glue <= 8.5f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.181271106005f ) {
                                    return 263.0/246.0;
                                } else {
                                    return 282.0/172.0;
                                }
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.420742124319f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.048400759697f ) {
                                        return 178.0/66.0;
                                    } else {
                                        if ( cl->stats.sum_uip1_used <= 1.5f ) {
                                            if ( cl->stats.glue_rel_queue <= 1.0448653698f ) {
                                                return 219.0/12.0;
                                            } else {
                                                return 274.0/58.0;
                                            }
                                        } else {
                                            return 193.0/52.0;
                                        }
                                    }
                                } else {
                                    return 299.0/126.0;
                                }
                            }
                        } else {
                            if ( cl->size() <= 13.5f ) {
                                return 332.0/94.0;
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                    return 305.0/22.0;
                                } else {
                                    return 239.0/40.0;
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.glue <= 10.5f ) {
                    if ( cl->stats.rdb1_last_touched_diff <= 66537.5f ) {
                        if ( cl->size() <= 13.5f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.67708337307f ) {
                                return 178.0/360.0;
                            } else {
                                return 229.0/180.0;
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 1.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.228298604488f ) {
                                    return 153.0/152.0;
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 75.5f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.318554520607f ) {
                                            return 152.0/104.0;
                                        } else {
                                            return 179.0/82.0;
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.650452911854f ) {
                                            return 205.0/58.0;
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 0.951877593994f ) {
                                                return 168.0/84.0;
                                            } else {
                                                return 183.0/54.0;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.dump_number <= 3.5f ) {
                                    return 219.0/280.0;
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 2.5f ) {
                                        return 220.0/130.0;
                                    } else {
                                        return 133.0/134.0;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.0249307472259f ) {
                            if ( cl->stats.glue_rel_long <= 0.700479745865f ) {
                                return 195.0/144.0;
                            } else {
                                return 219.0/90.0;
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 421320.0f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 8517.0f ) {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 8.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.855107069016f ) {
                                            if ( cl->stats.num_overlap_literals <= 34.5f ) {
                                                if ( cl->stats.num_antecedents_rel <= 0.304503440857f ) {
                                                    return 179.0/54.0;
                                                } else {
                                                    return 192.0/42.0;
                                                }
                                            } else {
                                                return 333.0/160.0;
                                            }
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 76.5f ) {
                                                return 250.0/38.0;
                                            } else {
                                                if ( cl->stats.size_rel <= 0.982561171055f ) {
                                                    return 179.0/42.0;
                                                } else {
                                                    return 207.0/68.0;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 209433.0f ) {
                                            return 190.0/42.0;
                                        } else {
                                            return 189.0/22.0;
                                        }
                                    }
                                } else {
                                    return 211.0/126.0;
                                }
                            } else {
                                return 340.0/38.0;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.size_rel <= 1.24345898628f ) {
                        if ( cl->stats.num_antecedents_rel <= 0.602686047554f ) {
                            if ( cl->stats.sum_uip1_used <= 2.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.1582018435f ) {
                                    if ( cl->size() <= 57.5f ) {
                                        return 275.0/114.0;
                                    } else {
                                        return 182.0/170.0;
                                    }
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 55.0f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 56262.5f ) {
                                            if ( cl->stats.num_overlap_literals <= 139.0f ) {
                                                if ( cl->size() <= 39.5f ) {
                                                    if ( cl->stats.glue_rel_queue <= 0.995415210724f ) {
                                                        return 186.0/64.0;
                                                    } else {
                                                        return 322.0/58.0;
                                                    }
                                                } else {
                                                    return 183.0/98.0;
                                                }
                                            } else {
                                                return 242.0/46.0;
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 1.26547455788f ) {
                                                if ( cl->stats.num_total_lits_antecedents <= 54.5f ) {
                                                    return 257.0/88.0;
                                                } else {
                                                    if ( cl->size() <= 41.5f ) {
                                                        if ( cl->stats.antecedents_glue_long_reds_var <= 12.7593688965f ) {
                                                            return 259.0/20.0;
                                                        } else {
                                                            return 245.0/38.0;
                                                        }
                                                    } else {
                                                        return 370.0/72.0;
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.glue <= 18.5f ) {
                                                    return 219.0/22.0;
                                                } else {
                                                    return 247.0/4.0;
                                                }
                                            }
                                        }
                                    } else {
                                        return 303.0/160.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.dump_number <= 8.5f ) {
                                    return 151.0/172.0;
                                } else {
                                    return 249.0/140.0;
                                }
                            }
                        } else {
                            if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 1.02835059166f ) {
                                    return 227.0/36.0;
                                } else {
                                    if ( cl->stats.size_rel <= 0.797308564186f ) {
                                        return 176.0/92.0;
                                    } else {
                                        return 269.0/76.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 27018.5f ) {
                                    return 307.0/88.0;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 110678.5f ) {
                                        if ( cl->stats.glue_rel_long <= 0.986279547215f ) {
                                            return 194.0/74.0;
                                        } else {
                                            if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                                return 349.0/18.0;
                                            } else {
                                                if ( cl->stats.glue_rel_long <= 1.2907075882f ) {
                                                    return 276.0/64.0;
                                                } else {
                                                    return 229.0/18.0;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.945412933826f ) {
                                            return 225.0/52.0;
                                        } else {
                                            if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                                if ( cl->stats.antec_num_total_lits_rel <= 1.7922513485f ) {
                                                    return 381.0/4.0;
                                                } else {
                                                    return 216.0/20.0;
                                                }
                                            } else {
                                                if ( cl->stats.size_rel <= 0.878655076027f ) {
                                                    return 217.0/46.0;
                                                } else {
                                                    if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                                        return 201.0/26.0;
                                                    } else {
                                                        return 240.0/12.0;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.334157586098f ) {
                            if ( cl->stats.size_rel <= 1.55253481865f ) {
                                return 224.0/56.0;
                            } else {
                                return 235.0/94.0;
                            }
                        } else {
                            if ( cl->stats.dump_number <= 8.5f ) {
                                if ( cl->stats.glue_rel_long <= 1.04310834408f ) {
                                    if ( cl->stats.size_rel <= 1.45710992813f ) {
                                        return 175.0/34.0;
                                    } else {
                                        return 346.0/130.0;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 10538.5f ) {
                                        return 359.0/32.0;
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 230.5f ) {
                                            if ( cl->stats.num_antecedents_rel <= 0.790118217468f ) {
                                                return 358.0/48.0;
                                            } else {
                                                return 183.0/86.0;
                                            }
                                        } else {
                                            if ( cl->stats.num_overlap_literals <= 199.0f ) {
                                                return 252.0/16.0;
                                            } else {
                                                if ( cl->stats.num_total_lits_antecedents <= 650.5f ) {
                                                    return 273.0/68.0;
                                                } else {
                                                    if ( cl->stats.rdb1_last_touched_diff <= 40916.0f ) {
                                                        return 198.0/12.0;
                                                    } else {
                                                        return 192.0/28.0;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.glue <= 20.5f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 55.5f ) {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 27.7399997711f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 109983.5f ) {
                                                return 224.0/42.0;
                                            } else {
                                                if ( cl->stats.glue <= 12.5f ) {
                                                    return 252.0/12.0;
                                                } else {
                                                    if ( cl->stats.antecedents_glue_long_reds_var <= 12.2700004578f ) {
                                                        if ( cl->stats.sum_delta_confl_uip1_used <= 1.5f ) {
                                                            return 241.0/14.0;
                                                        } else {
                                                            return 190.0/34.0;
                                                        }
                                                    } else {
                                                        return 350.0/20.0;
                                                    }
                                                }
                                            }
                                        } else {
                                            return 214.0/44.0;
                                        }
                                    } else {
                                        return 226.0/66.0;
                                    }
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 1.17893862724f ) {
                                        if ( rdb0_last_touched_diff <= 157067.0f ) {
                                            return 266.0/40.0;
                                        } else {
                                            return 363.0/14.0;
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 1.83074355125f ) {
                                            return 1;
                                        } else {
                                            if ( cl->size() <= 92.5f ) {
                                                return 1;
                                            } else {
                                                return 324.0/14.0;
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
                if ( cl->stats.num_overlap_literals <= 8.5f ) {
                    if ( cl->stats.size_rel <= 0.803371071815f ) {
                        if ( cl->stats.sum_uip1_used <= 76.5f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.glue <= 10.5f ) {
                                    if ( cl->stats.sum_uip1_used <= 15.5f ) {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 297852.0f ) {
                                            if ( cl->stats.glue_rel_long <= 0.568808555603f ) {
                                                return 126.0/604.0;
                                            } else {
                                                return 37.0/394.0;
                                            }
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 14.5f ) {
                                                return 72.0/306.0;
                                            } else {
                                                return 90.0/282.0;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.0417933650315f ) {
                                            return 71.0/286.0;
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 0.508041203022f ) {
                                                if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                                                    if ( cl->size() <= 5.5f ) {
                                                        return 73.0/396.0;
                                                    } else {
                                                        if ( cl->stats.sum_uip1_used <= 32.5f ) {
                                                            return 46.0/314.0;
                                                        } else {
                                                            return 43.0/358.0;
                                                        }
                                                    }
                                                } else {
                                                    return 37.0/470.0;
                                                }
                                            } else {
                                                if ( cl->stats.rdb1_last_touched_diff <= 6029.5f ) {
                                                    return 52.0/382.0;
                                                } else {
                                                    return 83.0/384.0;
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    return 95.0/262.0;
                                }
                            } else {
                                if ( cl->size() <= 7.5f ) {
                                    if ( cl->stats.dump_number <= 20.5f ) {
                                        if ( cl->stats.dump_number <= 7.5f ) {
                                            return 61.0/338.0;
                                        } else {
                                            return 73.0/302.0;
                                        }
                                    } else {
                                        return 125.0/362.0;
                                    }
                                } else {
                                    if ( cl->size() <= 17.5f ) {
                                        if ( cl->stats.sum_uip1_used <= 11.5f ) {
                                            return 129.0/260.0;
                                        } else {
                                            return 149.0/458.0;
                                        }
                                    } else {
                                        return 77.0/298.0;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.50783354044f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 40417.5f ) {
                                    if ( rdb0_last_touched_diff <= 17180.0f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 2482.0f ) {
                                            return 13.0/408.0;
                                        } else {
                                            return 40.0/677.9;
                                        }
                                    } else {
                                        if ( cl->size() <= 6.5f ) {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.0584522634745f ) {
                                                return 29.0/338.0;
                                            } else {
                                                return 28.0/528.0;
                                            }
                                        } else {
                                            return 53.0/440.0;
                                        }
                                    }
                                } else {
                                    return 63.0/286.0;
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 8372.5f ) {
                                    return 29.0/384.0;
                                } else {
                                    return 83.0/448.0;
                                }
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 23483.5f ) {
                            return 69.0/300.0;
                        } else {
                            return 111.0/198.0;
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_last_touched_diff <= 35065.5f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 223.5f ) {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 7948.0f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.087306573987f ) {
                                        return 104.0/262.0;
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 2252.5f ) {
                                            return 120.0/476.0;
                                        } else {
                                            if ( cl->stats.glue <= 9.5f ) {
                                                if ( cl->stats.sum_uip1_used <= 30.5f ) {
                                                    if ( rdb0_last_touched_diff <= 15040.5f ) {
                                                        return 78.0/256.0;
                                                    } else {
                                                        return 58.0/280.0;
                                                    }
                                                } else {
                                                    return 36.0/408.0;
                                                }
                                            } else {
                                                return 101.0/416.0;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 8.5f ) {
                                        if ( rdb0_last_touched_diff <= 23709.0f ) {
                                            return 216.0/294.0;
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.295383810997f ) {
                                                return 190.0/438.0;
                                            } else {
                                                return 146.0/260.0;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.sum_uip1_used <= 76.5f ) {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 8.07999992371f ) {
                                                if ( cl->stats.num_antecedents_rel <= 0.15729624033f ) {
                                                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                                        return 105.0/366.0;
                                                    } else {
                                                        return 160.0/330.0;
                                                    }
                                                } else {
                                                    if ( cl->stats.num_antecedents_rel <= 0.300599187613f ) {
                                                        if ( cl->stats.antec_num_total_lits_rel <= 0.223719328642f ) {
                                                            return 83.0/576.0;
                                                        } else {
                                                            return 103.0/236.0;
                                                        }
                                                    } else {
                                                        if ( cl->stats.sum_delta_confl_uip1_used <= 441866.0f ) {
                                                            return 65.0/292.0;
                                                        } else {
                                                            if ( cl->stats.antec_num_total_lits_rel <= 0.370158016682f ) {
                                                                return 118.0/274.0;
                                                            } else {
                                                                return 85.0/316.0;
                                                            }
                                                        }
                                                    }
                                                }
                                            } else {
                                                return 72.0/380.0;
                                            }
                                        } else {
                                            if ( rdb0_last_touched_diff <= 29377.5f ) {
                                                return 53.0/568.0;
                                            } else {
                                                return 68.0/364.0;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 11829.5f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 503862.0f ) {
                                        return 66.0/306.0;
                                    } else {
                                        return 39.0/452.0;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 10.5f ) {
                                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                            if ( cl->stats.dump_number <= 7.5f ) {
                                                return 80.0/272.0;
                                            } else {
                                                return 43.0/402.0;
                                            }
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.242423474789f ) {
                                                return 85.0/248.0;
                                            } else {
                                                return 67.0/328.0;
                                            }
                                        }
                                    } else {
                                        return 65.0/488.0;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.891859173775f ) {
                                return 174.0/490.0;
                            } else {
                                return 219.0/362.0;
                            }
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 4.24896717072f ) {
                            if ( cl->stats.sum_uip1_used <= 23.5f ) {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                    return 170.0/374.0;
                                } else {
                                    if ( cl->stats.dump_number <= 14.5f ) {
                                        return 152.0/248.0;
                                    } else {
                                        return 237.0/204.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.127373307943f ) {
                                    return 99.0/230.0;
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.571934342384f ) {
                                        return 79.0/332.0;
                                    } else {
                                        return 105.0/272.0;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.size_rel <= 0.883607625961f ) {
                                return 227.0/372.0;
                            } else {
                                return 214.0/148.0;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.sum_uip1_used <= 28.5f ) {
                    if ( cl->stats.size_rel <= 0.696466267109f ) {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 14.8081626892f ) {
                            if ( cl->stats.dump_number <= 25.5f ) {
                                if ( cl->stats.num_overlap_literals <= 103.5f ) {
                                    if ( rdb0_last_touched_diff <= 47347.0f ) {
                                        if ( cl->stats.dump_number <= 11.5f ) {
                                            if ( cl->stats.glue_rel_long <= 0.424795627594f ) {
                                                return 71.0/526.0;
                                            } else {
                                                if ( cl->size() <= 9.5f ) {
                                                    return 64.0/294.0;
                                                } else {
                                                    if ( rdb0_last_touched_diff <= 33098.5f ) {
                                                        return 89.0/316.0;
                                                    } else {
                                                        return 97.0/208.0;
                                                    }
                                                }
                                            }
                                        } else {
                                            return 127.0/224.0;
                                        }
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 1.06880664825f ) {
                                            if ( cl->stats.num_antecedents_rel <= 0.096229583025f ) {
                                                if ( cl->stats.num_overlap_literals <= 6.5f ) {
                                                    return 155.0/224.0;
                                                } else {
                                                    return 144.0/138.0;
                                                }
                                            } else {
                                                if ( cl->stats.num_total_lits_antecedents <= 22.5f ) {
                                                    if ( cl->stats.glue_rel_long <= 0.418318510056f ) {
                                                        return 117.0/248.0;
                                                    } else {
                                                        return 128.0/430.0;
                                                    }
                                                } else {
                                                    if ( cl->stats.rdb1_last_touched_diff <= 143822.5f ) {
                                                        if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                                            if ( cl->stats.num_total_lits_antecedents <= 34.5f ) {
                                                                return 109.0/242.0;
                                                            } else {
                                                                return 142.0/236.0;
                                                            }
                                                        } else {
                                                            return 97.0/322.0;
                                                        }
                                                    } else {
                                                        return 148.0/160.0;
                                                    }
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 3.97959184647f ) {
                                                return 225.0/218.0;
                                            } else {
                                                return 152.0/248.0;
                                            }
                                        }
                                    }
                                } else {
                                    return 181.0/158.0;
                                }
                            } else {
                                if ( cl->stats.sum_uip1_used <= 4.5f ) {
                                    return 187.0/102.0;
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.807369947433f ) {
                                        if ( cl->stats.glue <= 4.5f ) {
                                            if ( cl->stats.glue_rel_long <= 0.351780474186f ) {
                                                return 179.0/176.0;
                                            } else {
                                                return 114.0/190.0;
                                            }
                                        } else {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 196888.0f ) {
                                                return 237.0/128.0;
                                            } else {
                                                if ( cl->stats.num_antecedents_rel <= 0.188067257404f ) {
                                                    if ( cl->stats.num_overlap_literals <= 7.5f ) {
                                                        return 159.0/182.0;
                                                    } else {
                                                        return 207.0/120.0;
                                                    }
                                                } else {
                                                    if ( cl->stats.num_total_lits_antecedents <= 30.5f ) {
                                                        return 159.0/142.0;
                                                    } else {
                                                        return 181.0/262.0;
                                                    }
                                                }
                                            }
                                        }
                                    } else {
                                        return 277.0/170.0;
                                    }
                                }
                            }
                        } else {
                            return 288.0/148.0;
                        }
                    } else {
                        if ( cl->stats.glue_rel_long <= 0.661883950233f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 2.00826454163f ) {
                                return 201.0/366.0;
                            } else {
                                return 194.0/152.0;
                            }
                        } else {
                            if ( cl->stats.num_antecedents_rel <= 0.882518768311f ) {
                                if ( cl->stats.dump_number <= 37.5f ) {
                                    if ( cl->size() <= 52.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 60386.0f ) {
                                            return 221.0/278.0;
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.424413561821f ) {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.33603823185f ) {
                                                    return 190.0/132.0;
                                                } else {
                                                    return 218.0/94.0;
                                                }
                                            } else {
                                                return 182.0/144.0;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.sum_uip1_used <= 6.5f ) {
                                            return 202.0/142.0;
                                        } else {
                                            return 212.0/310.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 79.5f ) {
                                        return 258.0/120.0;
                                    } else {
                                        return 195.0/56.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 23020.5f ) {
                                    return 192.0/30.0;
                                } else {
                                    return 193.0/96.0;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_act_ranking_top_10 <= 6.5f ) {
                        if ( cl->stats.glue <= 6.5f ) {
                            if ( cl->stats.sum_uip1_used <= 58.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 64573.0f ) {
                                    return 96.0/456.0;
                                } else {
                                    return 143.0/262.0;
                                }
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 7771424.0f ) {
                                    return 77.0/522.0;
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0348538272083f ) {
                                        return 78.0/246.0;
                                    } else {
                                        return 63.0/426.0;
                                    }
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 55880.0f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 23301.0f ) {
                                    return 34.0/332.0;
                                } else {
                                    return 65.0/352.0;
                                }
                            } else {
                                if ( cl->stats.dump_number <= 37.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.704372882843f ) {
                                        return 122.0/314.0;
                                    } else {
                                        return 123.0/222.0;
                                    }
                                } else {
                                    return 216.0/344.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.dump_number <= 23.5f ) {
                            return 89.0/442.0;
                        } else {
                            if ( rdb0_last_touched_diff <= 202897.5f ) {
                                return 193.0/432.0;
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 384416.5f ) {
                                    if ( cl->stats.size_rel <= 0.377707093954f ) {
                                        return 129.0/154.0;
                                    } else {
                                        return 139.0/132.0;
                                    }
                                } else {
                                    return 113.0/224.0;
                                }
                            }
                        }
                    }
                }
            }
        }
    } else {
        if ( cl->stats.sum_uip1_used <= 14.5f ) {
            if ( cl->stats.rdb1_last_touched_diff <= 85127.0f ) {
                if ( cl->stats.antec_num_total_lits_rel <= 1.05123305321f ) {
                    if ( cl->size() <= 10.5f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 31.5f ) {
                            if ( cl->stats.sum_uip1_used <= 6.5f ) {
                                if ( cl->stats.dump_number <= 2.5f ) {
                                    return 71.0/596.0;
                                } else {
                                    return 107.0/424.0;
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.598821401596f ) {
                                    if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                                        return 54.0/392.0;
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 4469.0f ) {
                                            return 17.0/424.0;
                                        } else {
                                            return 56.0/532.0;
                                        }
                                    }
                                } else {
                                    return 59.0/390.0;
                                }
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.434567898512f ) {
                                return 85.0/290.0;
                            } else {
                                return 54.0/300.0;
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 11236.0f ) {
                            if ( cl->stats.size_rel <= 0.9827054739f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.0381944440305f ) {
                                    if ( rdb0_last_touched_diff <= 4651.5f ) {
                                        return 43.0/440.0;
                                    } else {
                                        return 58.0/282.0;
                                    }
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 6.5f ) {
                                        return 132.0/380.0;
                                    } else {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                            return 46.0/350.0;
                                        } else {
                                            return 65.0/338.0;
                                        }
                                    }
                                }
                            } else {
                                return 117.0/288.0;
                            }
                        } else {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.367815613747f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 143877.0f ) {
                                    return 174.0/346.0;
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 42.5f ) {
                                        return 72.0/284.0;
                                    } else {
                                        return 94.0/252.0;
                                    }
                                }
                            } else {
                                return 102.0/458.0;
                            }
                        }
                    }
                } else {
                    return 191.0/348.0;
                }
            } else {
                if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                    return 179.0/160.0;
                } else {
                    return 125.0/262.0;
                }
            }
        } else {
            if ( cl->stats.sum_uip1_used <= 55.5f ) {
                if ( cl->stats.rdb1_last_touched_diff <= 27556.0f ) {
                    if ( cl->stats.dump_number <= 10.5f ) {
                        if ( cl->stats.num_overlap_literals <= 20.5f ) {
                            if ( rdb0_last_touched_diff <= 2092.5f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.0598365366459f ) {
                                    return 36.0/771.9;
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.208003103733f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.062676101923f ) {
                                            return 13.0/745.9;
                                        } else {
                                            return 13.0/398.0;
                                        }
                                    } else {
                                        return 3.0/516.0;
                                    }
                                }
                            } else {
                                if ( cl->size() <= 24.5f ) {
                                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 2009.0f ) {
                                            return 51.0/707.9;
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.097553178668f ) {
                                                return 30.0/514.0;
                                            } else {
                                                return 18.0/743.9;
                                            }
                                        }
                                    } else {
                                        return 54.0/608.0;
                                    }
                                } else {
                                    return 46.0/412.0;
                                }
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 21.5f ) {
                                return 105.0/620.0;
                            } else {
                                if ( rdb0_last_touched_diff <= 3201.5f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 782742.0f ) {
                                        return 27.0/502.0;
                                    } else {
                                        return 43.0/400.0;
                                    }
                                } else {
                                    return 67.0/506.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_queue <= 0.887723922729f ) {
                            if ( rdb0_last_touched_diff <= 7527.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 4162.0f ) {
                                    if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                        return 25.0/598.0;
                                    } else {
                                        return 35.0/442.0;
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.103376232088f ) {
                                        return 69.0/358.0;
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.429919600487f ) {
                                            return 59.0/294.0;
                                        } else {
                                            return 47.0/661.9;
                                        }
                                    }
                                }
                            } else {
                                return 101.0/500.0;
                            }
                        } else {
                            return 97.0/388.0;
                        }
                    }
                } else {
                    if ( cl->stats.num_antecedents_rel <= 0.101998127997f ) {
                        return 85.0/284.0;
                    } else {
                        if ( rdb0_last_touched_diff <= 5463.5f ) {
                            if ( cl->size() <= 10.5f ) {
                                return 44.0/482.0;
                            } else {
                                return 86.0/516.0;
                            }
                        } else {
                            return 93.0/216.0;
                        }
                    }
                }
            } else {
                if ( cl->stats.dump_number <= 17.5f ) {
                    if ( cl->stats.antecedents_glue_long_reds_var <= 5.54947805405f ) {
                        if ( cl->stats.num_overlap_literals_rel <= 0.00327232806012f ) {
                            return 5.0/769.9;
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.162249907851f ) {
                                return 39.0/546.0;
                            } else {
                                if ( cl->stats.sum_uip1_used <= 245.0f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                                        if ( rdb0_last_touched_diff <= 1719.0f ) {
                                            return 12.0/418.0;
                                        } else {
                                            return 38.0/484.0;
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0181514322758f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 2316.0f ) {
                                                return 15.0/667.9;
                                            } else {
                                                return 32.0/346.0;
                                            }
                                        } else {
                                            if ( cl->stats.size_rel <= 0.413441061974f ) {
                                                if ( cl->stats.sum_uip1_used <= 65.5f ) {
                                                    return 21.0/516.0;
                                                } else {
                                                    if ( cl->size() <= 20.5f ) {
                                                        if ( cl->stats.glue_rel_queue <= 0.291333019733f ) {
                                                            return 13.0/416.0;
                                                        } else {
                                                            if ( cl->stats.num_overlap_literals <= 15.5f ) {
                                                                if ( cl->stats.used_for_uip_creation <= 15.5f ) {
                                                                    if ( cl->stats.num_antecedents_rel <= 0.137304127216f ) {
                                                                        return 9.0/438.0;
                                                                    } else {
                                                                        return 4.0/508.0;
                                                                    }
                                                                } else {
                                                                    return 2.0/654.0;
                                                                }
                                                            } else {
                                                                return 11.0/402.0;
                                                            }
                                                        }
                                                    } else {
                                                        return 3.0/418.0;
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.size_rel <= 0.656440138817f ) {
                                                    return 35.0/592.0;
                                                } else {
                                                    return 8.0/498.0;
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 97.0f ) {
                                        if ( cl->stats.dump_number <= 10.5f ) {
                                            return 2.0/697.9;
                                        } else {
                                            if ( cl->stats.glue <= 5.5f ) {
                                                return 12.0/428.0;
                                            } else {
                                                return 3.0/412.0;
                                            }
                                        }
                                    } else {
                                        return 10.0/388.0;
                                    }
                                }
                            }
                        }
                    } else {
                        return 34.0/544.0;
                    }
                } else {
                    if ( cl->stats.rdb1_last_touched_diff <= 9379.5f ) {
                        if ( cl->size() <= 53.5f ) {
                            if ( cl->stats.sum_uip1_used <= 80.5f ) {
                                return 57.0/703.9;
                            } else {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                    if ( cl->stats.size_rel <= 0.244936794043f ) {
                                        return 29.0/352.0;
                                    } else {
                                        return 17.0/406.0;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.228397712111f ) {
                                        if ( cl->stats.used_for_uip_creation <= 10.5f ) {
                                            return 31.0/358.0;
                                        } else {
                                            return 14.0/474.0;
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 8.5f ) {
                                            if ( cl->stats.num_antecedents_rel <= 0.0322558656335f ) {
                                                return 22.0/398.0;
                                            } else {
                                                if ( cl->stats.glue_rel_queue <= 0.299177408218f ) {
                                                    return 14.0/600.0;
                                                } else {
                                                    if ( cl->stats.sum_uip1_used <= 798.5f ) {
                                                        if ( cl->stats.glue_rel_long <= 0.503798246384f ) {
                                                            if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                                                return 0.0/480.0;
                                                            } else {
                                                                return 4.0/745.9;
                                                            }
                                                        } else {
                                                            return 9.0/729.9;
                                                        }
                                                    } else {
                                                        return 8.0/384.0;
                                                    }
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.num_overlap_literals <= 18.5f ) {
                                                if ( cl->stats.glue_rel_queue <= 0.401181608438f ) {
                                                    return 16.0/448.0;
                                                } else {
                                                    return 38.0/715.9;
                                                }
                                            } else {
                                                if ( cl->stats.sum_delta_confl_uip1_used <= 68508944.0f ) {
                                                    return 5.0/436.0;
                                                } else {
                                                    return 9.0/368.0;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            return 47.0/508.0;
                        }
                    } else {
                        if ( cl->stats.num_antecedents_rel <= 0.0937786102295f ) {
                            if ( cl->stats.num_overlap_literals_rel <= 0.0222490206361f ) {
                                return 35.0/368.0;
                            } else {
                                return 63.0/392.0;
                            }
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                                return 57.0/510.0;
                            } else {
                                if ( cl->stats.sum_uip1_used <= 271.0f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 21.5f ) {
                                        return 21.0/496.0;
                                    } else {
                                        return 48.0/546.0;
                                    }
                                } else {
                                    return 9.0/460.0;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

static double estimator_should_keep_long_conf4_cluster0_2(
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
    if ( cl->stats.size_rel <= 0.618443667889f ) {
        if ( cl->stats.sum_uip1_used <= 5.5f ) {
            if ( cl->stats.glue_rel_queue <= 0.847641825676f ) {
                if ( cl->stats.num_total_lits_antecedents <= 19.5f ) {
                    if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 150331.5f ) {
                            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0250220932066f ) {
                                    return 44.0/450.0;
                                } else {
                                    return 100.0/452.0;
                                }
                            } else {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                                    if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 25555.0f ) {
                                            return 103.0/284.0;
                                        } else {
                                            return 195.0/278.0;
                                        }
                                    } else {
                                        return 109.0/304.0;
                                    }
                                } else {
                                    return 96.0/384.0;
                                }
                            }
                        } else {
                            return 145.0/248.0;
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 90344.0f ) {
                            return 191.0/358.0;
                        } else {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 59.5f ) {
                                return 237.0/116.0;
                            } else {
                                return 197.0/250.0;
                            }
                        }
                    }
                } else {
                    if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                            if ( cl->stats.num_overlap_literals <= 58.5f ) {
                                if ( cl->size() <= 9.5f ) {
                                    return 134.0/540.0;
                                } else {
                                    if ( cl->stats.dump_number <= 5.5f ) {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                                            return 155.0/308.0;
                                        } else {
                                            return 87.0/298.0;
                                        }
                                    } else {
                                        return 112.0/188.0;
                                    }
                                }
                            } else {
                                return 149.0/248.0;
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 42998.5f ) {
                                if ( cl->stats.sum_uip1_used <= 1.5f ) {
                                    return 180.0/246.0;
                                } else {
                                    if ( cl->stats.dump_number <= 3.5f ) {
                                        return 116.0/458.0;
                                    } else {
                                        return 112.0/208.0;
                                    }
                                }
                            } else {
                                return 235.0/188.0;
                            }
                        }
                    } else {
                        if ( cl->stats.dump_number <= 6.5f ) {
                            if ( cl->size() <= 9.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 27275.0f ) {
                                    return 141.0/362.0;
                                } else {
                                    return 143.0/202.0;
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.74162364006f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 83.5f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.122481375933f ) {
                                            return 148.0/180.0;
                                        } else {
                                            return 187.0/438.0;
                                        }
                                    } else {
                                        return 187.0/188.0;
                                    }
                                } else {
                                    return 205.0/142.0;
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 159647.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 1.03211736679f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0791819319129f ) {
                                        return 154.0/108.0;
                                    } else {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 39.0f ) {
                                            return 212.0/156.0;
                                        } else {
                                            if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                                return 201.0/216.0;
                                            } else {
                                                return 115.0/246.0;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 60.5f ) {
                                        return 275.0/186.0;
                                    } else {
                                        return 186.0/66.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.391139447689f ) {
                                    if ( cl->stats.size_rel <= 0.170830190182f ) {
                                        return 137.0/132.0;
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.153236091137f ) {
                                            return 341.0/76.0;
                                        } else {
                                            if ( cl->size() <= 12.5f ) {
                                                return 199.0/96.0;
                                            } else {
                                                return 227.0/60.0;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 32.5f ) {
                                        return 148.0/128.0;
                                    } else {
                                        return 229.0/144.0;
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.sum_uip1_used <= 1.5f ) {
                    if ( cl->stats.num_overlap_literals <= 23.5f ) {
                        if ( rdb0_last_touched_diff <= 31593.0f ) {
                            return 111.0/238.0;
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 34.5f ) {
                                return 255.0/262.0;
                            } else {
                                return 176.0/40.0;
                            }
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 4.7282166481f ) {
                            if ( rdb0_last_touched_diff <= 70559.0f ) {
                                return 307.0/188.0;
                            } else {
                                if ( cl->stats.size_rel <= 0.420249044895f ) {
                                    return 194.0/18.0;
                                } else {
                                    return 180.0/46.0;
                                }
                            }
                        } else {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 0.5f ) {
                                return 177.0/62.0;
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.908490180969f ) {
                                    return 359.0/114.0;
                                } else {
                                    if ( cl->stats.glue <= 15.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 57617.0f ) {
                                            return 283.0/70.0;
                                        } else {
                                            return 318.0/40.0;
                                        }
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.561102330685f ) {
                                            return 226.0/26.0;
                                        } else {
                                            if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                                return 270.0/6.0;
                                            } else {
                                                return 268.0/12.0;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.glue <= 8.5f ) {
                        if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 19011.0f ) {
                                return 86.0/298.0;
                            } else {
                                return 108.0/210.0;
                            }
                        } else {
                            return 261.0/254.0;
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 151.5f ) {
                            if ( cl->size() <= 13.5f ) {
                                return 121.0/186.0;
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.133984774351f ) {
                                    return 219.0/186.0;
                                } else {
                                    return 205.0/114.0;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 30349.0f ) {
                                return 162.0/84.0;
                            } else {
                                return 314.0/78.0;
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.sum_delta_confl_uip1_used <= 3606765.0f ) {
                if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                    if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.293846607208f ) {
                            if ( rdb0_last_touched_diff <= 6781.0f ) {
                                if ( cl->stats.dump_number <= 15.5f ) {
                                    if ( cl->stats.used_for_uip_creation <= 5.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0840571075678f ) {
                                            return 66.0/508.0;
                                        } else {
                                            if ( cl->stats.num_overlap_literals <= 7.5f ) {
                                                return 18.0/438.0;
                                            } else {
                                                return 33.0/372.0;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.438048243523f ) {
                                            return 21.0/414.0;
                                        } else {
                                            return 14.0/424.0;
                                        }
                                    }
                                } else {
                                    return 118.0/514.0;
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 34766.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.224984288216f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.378501355648f ) {
                                            if ( cl->size() <= 5.5f ) {
                                                return 60.0/358.0;
                                            } else {
                                                return 117.0/330.0;
                                            }
                                        } else {
                                            if ( cl->stats.dump_number <= 12.5f ) {
                                                if ( rdb0_last_touched_diff <= 24000.5f ) {
                                                    return 35.0/354.0;
                                                } else {
                                                    return 72.0/440.0;
                                                }
                                            } else {
                                                return 108.0/516.0;
                                            }
                                        }
                                    } else {
                                        return 83.0/216.0;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0500907897949f ) {
                                        return 149.0/450.0;
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 11.5f ) {
                                            return 88.0/226.0;
                                        } else {
                                            return 122.0/210.0;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->size() <= 9.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 1.62152779102f ) {
                                    return 99.0/342.0;
                                } else {
                                    return 61.0/446.0;
                                }
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                    if ( cl->stats.sum_uip1_used <= 20.5f ) {
                                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.11482565105f ) {
                                                return 82.0/260.0;
                                            } else {
                                                return 138.0/326.0;
                                            }
                                        } else {
                                            return 189.0/278.0;
                                        }
                                    } else {
                                        return 137.0/550.0;
                                    }
                                } else {
                                    return 64.0/460.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 62018.0f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 28408.5f ) {
                                if ( cl->stats.glue_rel_long <= 0.682707726955f ) {
                                    if ( cl->stats.dump_number <= 12.5f ) {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 383181.0f ) {
                                            if ( rdb0_last_touched_diff <= 28457.5f ) {
                                                return 72.0/342.0;
                                            } else {
                                                return 48.0/378.0;
                                            }
                                        } else {
                                            return 35.0/404.0;
                                        }
                                    } else {
                                        return 89.0/300.0;
                                    }
                                } else {
                                    return 118.0/410.0;
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.715595722198f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.033671874553f ) {
                                        if ( cl->stats.dump_number <= 16.5f ) {
                                            if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                                return 83.0/290.0;
                                            } else {
                                                return 82.0/436.0;
                                            }
                                        } else {
                                            return 111.0/204.0;
                                        }
                                    } else {
                                        return 176.0/402.0;
                                    }
                                } else {
                                    return 171.0/268.0;
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 273134.0f ) {
                                if ( cl->stats.num_overlap_literals <= 70.5f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.124017059803f ) {
                                        if ( cl->stats.dump_number <= 18.5f ) {
                                            if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                                return 136.0/210.0;
                                            } else {
                                                if ( cl->stats.num_overlap_literals <= 6.5f ) {
                                                    return 89.0/282.0;
                                                } else {
                                                    return 118.0/242.0;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.size_rel <= 0.156417399645f ) {
                                                return 150.0/244.0;
                                            } else {
                                                if ( cl->stats.num_overlap_literals <= 7.5f ) {
                                                    if ( cl->stats.size_rel <= 0.344931155443f ) {
                                                        return 152.0/166.0;
                                                    } else {
                                                        return 140.0/270.0;
                                                    }
                                                } else {
                                                    return 276.0/264.0;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.562660813332f ) {
                                            return 147.0/426.0;
                                        } else {
                                            if ( cl->stats.sum_uip1_used <= 12.5f ) {
                                                return 159.0/182.0;
                                            } else {
                                                return 93.0/238.0;
                                            }
                                        }
                                    }
                                } else {
                                    return 195.0/180.0;
                                }
                            } else {
                                if ( cl->stats.size_rel <= 0.219158887863f ) {
                                    return 164.0/204.0;
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 34.5f ) {
                                        return 190.0/144.0;
                                    } else {
                                        return 226.0/106.0;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_overlap_literals <= 6.5f ) {
                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                            if ( cl->stats.dump_number <= 12.5f ) {
                                if ( cl->stats.sum_uip1_used <= 52.5f ) {
                                    if ( rdb0_last_touched_diff <= 8127.0f ) {
                                        if ( cl->stats.sum_uip1_used <= 13.5f ) {
                                            return 45.0/574.0;
                                        } else {
                                            if ( cl->stats.size_rel <= 0.126103281975f ) {
                                                return 8.0/406.0;
                                            } else {
                                                if ( cl->stats.num_overlap_literals_rel <= 0.0300496201962f ) {
                                                    return 36.0/500.0;
                                                } else {
                                                    if ( cl->stats.num_overlap_literals_rel <= 0.0566783286631f ) {
                                                        return 11.0/424.0;
                                                    } else {
                                                        return 17.0/402.0;
                                                    }
                                                }
                                            }
                                        }
                                    } else {
                                        return 86.0/612.0;
                                    }
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 102.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0378631427884f ) {
                                            return 5.0/570.0;
                                        } else {
                                            return 9.0/394.0;
                                        }
                                    } else {
                                        return 19.0/392.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.426664352417f ) {
                                    return 56.0/302.0;
                                } else {
                                    return 33.0/328.0;
                                }
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals_rel <= 0.0134669151157f ) {
                                return 69.0/546.0;
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 11.5f ) {
                                    return 37.0/614.0;
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 23.5f ) {
                                        return 67.0/458.0;
                                    } else {
                                        return 20.0/372.0;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                            if ( cl->stats.sum_uip1_used <= 11.5f ) {
                                if ( cl->stats.dump_number <= 7.5f ) {
                                    if ( cl->stats.num_overlap_literals <= 19.5f ) {
                                        return 66.0/330.0;
                                    } else {
                                        return 74.0/238.0;
                                    }
                                } else {
                                    return 126.0/262.0;
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 17867.5f ) {
                                    if ( cl->size() <= 10.5f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.260142505169f ) {
                                            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                                return 39.0/338.0;
                                            } else {
                                                return 44.0/322.0;
                                            }
                                        } else {
                                            return 23.0/390.0;
                                        }
                                    } else {
                                        if ( cl->stats.sum_uip1_used <= 25.5f ) {
                                            return 100.0/398.0;
                                        } else {
                                            return 58.0/378.0;
                                        }
                                    }
                                } else {
                                    return 75.0/294.0;
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.597051978111f ) {
                                if ( cl->stats.glue_rel_long <= 0.297914862633f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 11.5f ) {
                                        if ( cl->stats.sum_uip1_used <= 24.5f ) {
                                            return 51.0/330.0;
                                        } else {
                                            return 34.0/376.0;
                                        }
                                    } else {
                                        return 18.0/420.0;
                                    }
                                } else {
                                    if ( cl->size() <= 5.5f ) {
                                        return 21.0/578.0;
                                    } else {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 1860825.0f ) {
                                            if ( cl->stats.glue <= 4.5f ) {
                                                return 48.0/424.0;
                                            } else {
                                                if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                                    return 46.0/400.0;
                                                } else {
                                                    if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                                        return 31.0/356.0;
                                                    } else {
                                                        if ( cl->size() <= 13.5f ) {
                                                            return 5.0/384.0;
                                                        } else {
                                                            return 19.0/390.0;
                                                        }
                                                    }
                                                }
                                            }
                                        } else {
                                            return 16.0/522.0;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->size() <= 8.5f ) {
                                    return 43.0/697.9;
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 25.5f ) {
                                        if ( cl->stats.dump_number <= 4.5f ) {
                                            return 86.0/514.0;
                                        } else {
                                            return 80.0/302.0;
                                        }
                                    } else {
                                        if ( cl->stats.size_rel <= 0.385541647673f ) {
                                            return 24.0/440.0;
                                        } else {
                                            return 36.0/366.0;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                        if ( rdb0_last_touched_diff <= 8080.5f ) {
                            if ( cl->stats.sum_uip1_used <= 36.5f ) {
                                return 88.0/448.0;
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                                    if ( cl->stats.sum_uip1_used <= 93.5f ) {
                                        return 74.0/528.0;
                                    } else {
                                        if ( cl->stats.size_rel <= 0.206629425287f ) {
                                            return 35.0/356.0;
                                        } else {
                                            return 23.0/562.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 8043.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0533034801483f ) {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.045792542398f ) {
                                                if ( cl->stats.num_overlap_literals_rel <= 0.00361169176176f ) {
                                                    return 3.0/552.0;
                                                } else {
                                                    if ( cl->stats.sum_uip1_used <= 501.5f ) {
                                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0334627255797f ) {
                                                            if ( cl->stats.rdb1_last_touched_diff <= 1431.5f ) {
                                                                return 29.0/540.0;
                                                            } else {
                                                                return 42.0/472.0;
                                                            }
                                                        } else {
                                                            return 11.0/392.0;
                                                        }
                                                    } else {
                                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0254274364561f ) {
                                                            return 5.0/464.0;
                                                        } else {
                                                            return 10.0/374.0;
                                                        }
                                                    }
                                                }
                                            } else {
                                                return 34.0/384.0;
                                            }
                                        } else {
                                            if ( cl->stats.glue <= 4.5f ) {
                                                if ( cl->stats.num_total_lits_antecedents <= 20.5f ) {
                                                    if ( cl->stats.size_rel <= 0.203800112009f ) {
                                                        if ( cl->stats.glue_rel_queue <= 0.293255746365f ) {
                                                            return 0.0/432.0;
                                                        } else {
                                                            return 16.0/580.0;
                                                        }
                                                    } else {
                                                        if ( cl->stats.glue_rel_queue <= 0.319920301437f ) {
                                                            return 3.0/402.0;
                                                        } else {
                                                            return 1.0/618.0;
                                                        }
                                                    }
                                                } else {
                                                    return 16.0/719.9;
                                                }
                                            } else {
                                                if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                                    if ( cl->stats.num_overlap_literals <= 8.5f ) {
                                                        return 11.0/418.0;
                                                    } else {
                                                        return 28.0/432.0;
                                                    }
                                                } else {
                                                    if ( cl->stats.num_overlap_literals <= 4.5f ) {
                                                        return 2.0/548.0;
                                                    } else {
                                                        if ( cl->stats.num_overlap_literals_rel <= 0.0426057465374f ) {
                                                            return 24.0/452.0;
                                                        } else {
                                                            if ( cl->stats.num_antecedents_rel <= 0.23615065217f ) {
                                                                if ( cl->stats.antec_num_total_lits_rel <= 0.117492519319f ) {
                                                                    return 4.0/504.0;
                                                                } else {
                                                                    return 31.0/632.0;
                                                                }
                                                            } else {
                                                                if ( cl->stats.rdb1_used_for_uip_creation <= 20.5f ) {
                                                                    return 3.0/608.0;
                                                                } else {
                                                                    return 10.0/412.0;
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.349516153336f ) {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.0576933436096f ) {
                                                return 44.0/390.0;
                                            } else {
                                                return 35.0/532.0;
                                            }
                                        } else {
                                            if ( cl->stats.dump_number <= 19.5f ) {
                                                return 10.0/468.0;
                                            } else {
                                                if ( cl->stats.glue_rel_long <= 0.463841825724f ) {
                                                    return 16.0/464.0;
                                                } else {
                                                    if ( cl->stats.dump_number <= 39.5f ) {
                                                        return 29.0/336.0;
                                                    } else {
                                                        return 24.0/446.0;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 39.5f ) {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.296428084373f ) {
                                        return 83.0/406.0;
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.452617794275f ) {
                                            return 70.0/663.9;
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.159160256386f ) {
                                                return 57.0/508.0;
                                            } else {
                                                return 67.0/288.0;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 10.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.115649037063f ) {
                                            if ( cl->stats.glue_rel_long <= 0.410790801048f ) {
                                                return 45.0/648.0;
                                            } else {
                                                return 51.0/378.0;
                                            }
                                        } else {
                                            return 18.0/486.0;
                                        }
                                    } else {
                                        return 42.0/354.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.193364560604f ) {
                                    return 129.0/300.0;
                                } else {
                                    return 57.0/388.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 46246.0f ) {
                            if ( cl->stats.sum_uip1_used <= 77.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0554946325719f ) {
                                    return 153.0/380.0;
                                } else {
                                    return 105.0/518.0;
                                }
                            } else {
                                if ( cl->stats.dump_number <= 28.5f ) {
                                    if ( rdb0_last_touched_diff <= 11317.0f ) {
                                        return 11.0/661.9;
                                    } else {
                                        return 34.0/550.0;
                                    }
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 76566392.0f ) {
                                        if ( cl->stats.size_rel <= 0.26868802309f ) {
                                            return 65.0/378.0;
                                        } else {
                                            return 38.0/360.0;
                                        }
                                    } else {
                                        return 25.0/428.0;
                                    }
                                }
                            }
                        } else {
                            return 175.0/486.0;
                        }
                    }
                } else {
                    if ( cl->size() <= 4.5f ) {
                        if ( cl->stats.sum_uip1_used <= 109.5f ) {
                            return 73.0/240.0;
                        } else {
                            return 46.0/342.0;
                        }
                    } else {
                        if ( cl->stats.dump_number <= 38.5f ) {
                            if ( rdb0_last_touched_diff <= 29248.5f ) {
                                return 31.0/476.0;
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.104506954551f ) {
                                    return 100.0/270.0;
                                } else {
                                    return 94.0/508.0;
                                }
                            }
                        } else {
                            if ( cl->size() <= 9.5f ) {
                                if ( rdb0_last_touched_diff <= 180256.0f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0636050999165f ) {
                                        return 99.0/250.0;
                                    } else {
                                        return 85.0/282.0;
                                    }
                                } else {
                                    return 148.0/228.0;
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 128936.5f ) {
                                    return 115.0/230.0;
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.176476955414f ) {
                                        return 153.0/122.0;
                                    } else {
                                        return 115.0/154.0;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    } else {
        if ( cl->stats.sum_uip1_used <= 4.5f ) {
            if ( cl->stats.sum_delta_confl_uip1_used <= 54.5f ) {
                if ( cl->stats.glue_rel_long <= 0.881749749184f ) {
                    if ( cl->size() <= 13.5f ) {
                        return 231.0/190.0;
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 191644.5f ) {
                            if ( cl->size() <= 19.5f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 2.5f ) {
                                    return 314.0/140.0;
                                } else {
                                    return 159.0/150.0;
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.294027119875f ) {
                                    return 279.0/184.0;
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.924878001213f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 108840.5f ) {
                                            if ( cl->stats.glue_rel_long <= 0.758560299873f ) {
                                                if ( cl->stats.rdb1_last_touched_diff <= 29642.5f ) {
                                                    return 209.0/102.0;
                                                } else {
                                                    return 254.0/74.0;
                                                }
                                            } else {
                                                return 297.0/170.0;
                                            }
                                        } else {
                                            return 242.0/72.0;
                                        }
                                    } else {
                                        return 219.0/30.0;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.75018799305f ) {
                                return 331.0/78.0;
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 80.5f ) {
                                    return 223.0/22.0;
                                } else {
                                    return 235.0/36.0;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_act_ranking_top_10 <= 8.5f ) {
                        if ( cl->stats.glue <= 13.5f ) {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 0.5f ) {
                                return 246.0/128.0;
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 46517.0f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 138.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.40691524744f ) {
                                            return 145.0/128.0;
                                        } else {
                                            if ( cl->stats.num_overlap_literals <= 47.5f ) {
                                                return 217.0/72.0;
                                            } else {
                                                return 189.0/122.0;
                                            }
                                        }
                                    } else {
                                        return 334.0/56.0;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.862890005112f ) {
                                        if ( cl->stats.num_overlap_literals <= 19.5f ) {
                                            return 268.0/74.0;
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 114.5f ) {
                                                if ( cl->stats.num_total_lits_antecedents <= 61.5f ) {
                                                    return 211.0/8.0;
                                                } else {
                                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 5.5f ) {
                                                        return 226.0/18.0;
                                                    } else {
                                                        return 207.0/36.0;
                                                    }
                                                }
                                            } else {
                                                return 188.0/48.0;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_act_ranking_top_10 <= 4.5f ) {
                                            return 244.0/36.0;
                                        } else {
                                            if ( cl->stats.num_overlap_literals_rel <= 1.4855890274f ) {
                                                return 186.0/56.0;
                                            } else {
                                                return 180.0/72.0;
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.892567574978f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 2.5f ) {
                                    if ( cl->stats.dump_number <= 4.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.471484839916f ) {
                                            return 298.0/144.0;
                                        } else {
                                            return 231.0/50.0;
                                        }
                                    } else {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 1.5f ) {
                                            if ( cl->stats.glue <= 26.5f ) {
                                                if ( cl->stats.glue_rel_queue <= 1.09062588215f ) {
                                                    return 189.0/38.0;
                                                } else {
                                                    if ( cl->size() <= 32.5f ) {
                                                        return 213.0/24.0;
                                                    } else {
                                                        return 213.0/34.0;
                                                    }
                                                }
                                            } else {
                                                return 345.0/20.0;
                                            }
                                        } else {
                                            return 263.0/56.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 3.46489810944f ) {
                                        return 177.0/64.0;
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 57.5f ) {
                                            return 258.0/14.0;
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 1.13225519657f ) {
                                                return 197.0/38.0;
                                            } else {
                                                return 296.0/18.0;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.size_rel <= 0.744422495365f ) {
                                    return 272.0/4.0;
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 0.5f ) {
                                        return 355.0/76.0;
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 1.10380792618f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 59898.0f ) {
                                                if ( cl->stats.num_antecedents_rel <= 0.607473909855f ) {
                                                    return 226.0/34.0;
                                                } else {
                                                    return 353.0/94.0;
                                                }
                                            } else {
                                                if ( cl->stats.rdb1_last_touched_diff <= 90890.0f ) {
                                                    return 193.0/16.0;
                                                } else {
                                                    if ( cl->stats.num_overlap_literals <= 157.0f ) {
                                                        return 282.0/4.0;
                                                    } else {
                                                        return 289.0/18.0;
                                                    }
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 1.66276431084f ) {
                                                if ( cl->stats.sum_delta_confl_uip1_used <= 3.5f ) {
                                                    if ( rdb0_last_touched_diff <= 60263.0f ) {
                                                        return 299.0/2.0;
                                                    } else {
                                                        if ( cl->stats.sum_delta_confl_uip1_used <= 1.5f ) {
                                                            return 319.0/10.0;
                                                        } else {
                                                            return 237.0/22.0;
                                                        }
                                                    }
                                                } else {
                                                    if ( rdb0_last_touched_diff <= 99627.5f ) {
                                                        return 280.0/60.0;
                                                    } else {
                                                        return 256.0/14.0;
                                                    }
                                                }
                                            } else {
                                                return 371.0/4.0;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 151143.0f ) {
                            if ( cl->stats.num_overlap_literals <= 123.0f ) {
                                return 246.0/44.0;
                            } else {
                                return 287.0/18.0;
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 2.55165290833f ) {
                                return 286.0/4.0;
                            } else {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 7.12152767181f ) {
                                    return 192.0/22.0;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 333657.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 1.15301275253f ) {
                                            return 336.0/2.0;
                                        } else {
                                            return 287.0/6.0;
                                        }
                                    } else {
                                        return 192.0/20.0;
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                    if ( cl->stats.glue <= 9.5f ) {
                        if ( rdb0_last_touched_diff <= 70149.5f ) {
                            if ( rdb0_last_touched_diff <= 15646.5f ) {
                                return 180.0/380.0;
                            } else {
                                if ( rdb0_last_touched_diff <= 32504.5f ) {
                                    return 157.0/156.0;
                                } else {
                                    return 117.0/226.0;
                                }
                            }
                        } else {
                            return 151.0/122.0;
                        }
                    } else {
                        if ( cl->stats.num_overlap_literals <= 284.5f ) {
                            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 26907.0f ) {
                                    return 169.0/284.0;
                                } else {
                                    return 157.0/132.0;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 26.5f ) {
                                    return 230.0/240.0;
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.485261678696f ) {
                                        return 272.0/108.0;
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 1.04258179665f ) {
                                            return 197.0/176.0;
                                        } else {
                                            return 151.0/90.0;
                                        }
                                    }
                                }
                            }
                        } else {
                            return 261.0/70.0;
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_last_touched_diff <= 51068.5f ) {
                        return 247.0/210.0;
                    } else {
                        if ( cl->stats.num_overlap_literals_rel <= 0.178881824017f ) {
                            if ( cl->stats.glue_rel_long <= 0.784846186638f ) {
                                return 159.0/134.0;
                            } else {
                                return 208.0/94.0;
                            }
                        } else {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 39317.0f ) {
                                if ( cl->stats.sum_uip1_used <= 2.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.710683584213f ) {
                                        return 204.0/20.0;
                                    } else {
                                        return 279.0/54.0;
                                    }
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.761228978634f ) {
                                        return 233.0/78.0;
                                    } else {
                                        return 208.0/42.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.962441205978f ) {
                                    return 169.0/90.0;
                                } else {
                                    return 188.0/50.0;
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.sum_uip1_used <= 19.5f ) {
                if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                    if ( cl->stats.num_antecedents_rel <= 1.22162902355f ) {
                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                            if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 41445.0f ) {
                                    if ( cl->stats.size_rel <= 1.05439996719f ) {
                                        if ( cl->stats.glue_rel_long <= 0.952354371548f ) {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                                if ( cl->stats.glue_rel_queue <= 0.697438001633f ) {
                                                    return 74.0/304.0;
                                                } else {
                                                    return 53.0/330.0;
                                                }
                                            } else {
                                                return 105.0/378.0;
                                            }
                                        } else {
                                            return 85.0/208.0;
                                        }
                                    } else {
                                        return 174.0/434.0;
                                    }
                                } else {
                                    return 185.0/304.0;
                                }
                            } else {
                                return 47.0/416.0;
                            }
                        } else {
                            if ( cl->stats.dump_number <= 23.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.203210324049f ) {
                                    return 152.0/352.0;
                                } else {
                                    if ( rdb0_last_touched_diff <= 14829.5f ) {
                                        return 50.0/326.0;
                                    } else {
                                        return 130.0/320.0;
                                    }
                                }
                            } else {
                                return 143.0/164.0;
                            }
                        }
                    } else {
                        return 163.0/208.0;
                    }
                } else {
                    if ( cl->stats.sum_delta_confl_uip1_used <= 10240.0f ) {
                        if ( cl->stats.antec_num_total_lits_rel <= 0.437787890434f ) {
                            return 251.0/234.0;
                        } else {
                            if ( cl->size() <= 45.0f ) {
                                return 192.0/60.0;
                            } else {
                                return 182.0/82.0;
                            }
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.67708337307f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 67664.5f ) {
                                if ( rdb0_last_touched_diff <= 33897.0f ) {
                                    return 115.0/190.0;
                                } else {
                                    return 87.0/332.0;
                                }
                            } else {
                                if ( cl->stats.glue <= 9.5f ) {
                                    return 157.0/196.0;
                                } else {
                                    return 236.0/182.0;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 4.5f ) {
                                if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                    return 233.0/356.0;
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.2626542449f ) {
                                        return 166.0/128.0;
                                    } else {
                                        return 185.0/244.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 276642.5f ) {
                                    if ( cl->size() <= 31.5f ) {
                                        return 178.0/124.0;
                                    } else {
                                        return 138.0/140.0;
                                    }
                                } else {
                                    return 285.0/128.0;
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.sum_uip1_used <= 46.5f ) {
                    if ( cl->stats.num_overlap_literals <= 69.5f ) {
                        if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 11034.5f ) {
                                if ( cl->size() <= 32.5f ) {
                                    return 57.0/572.0;
                                } else {
                                    return 74.0/292.0;
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 116631.0f ) {
                                    if ( cl->stats.sum_uip1_used <= 33.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.329306185246f ) {
                                            return 116.0/206.0;
                                        } else {
                                            return 88.0/232.0;
                                        }
                                    } else {
                                        return 82.0/294.0;
                                    }
                                } else {
                                    return 158.0/192.0;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 11752.5f ) {
                                return 34.0/332.0;
                            } else {
                                return 33.0/412.0;
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 897197.5f ) {
                                return 122.0/172.0;
                            } else {
                                return 122.0/236.0;
                            }
                        } else {
                            return 86.0/320.0;
                        }
                    }
                } else {
                    if ( rdb0_last_touched_diff <= 45813.0f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 6104.0f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 62.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.156598240137f ) {
                                    return 25.0/462.0;
                                } else {
                                    return 16.0/701.9;
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 5746.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 148.5f ) {
                                        return 28.0/480.0;
                                    } else {
                                        return 10.0/422.0;
                                    }
                                } else {
                                    return 37.0/416.0;
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 4968.5f ) {
                                return 42.0/675.9;
                            } else {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                    return 77.0/402.0;
                                } else {
                                    return 64.0/472.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->size() <= 41.5f ) {
                            return 120.0/356.0;
                        } else {
                            return 127.0/208.0;
                        }
                    }
                }
            }
        }
    }
}

static double estimator_should_keep_long_conf4_cluster0_3(
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
        if ( cl->stats.glue_rel_long <= 0.812404930592f ) {
            if ( cl->stats.sum_uip1_used <= 7.5f ) {
                if ( rdb0_last_touched_diff <= 16789.5f ) {
                    if ( rdb0_last_touched_diff <= 8368.0f ) {
                        if ( cl->size() <= 9.5f ) {
                            if ( cl->stats.glue_rel_long <= 0.49600854516f ) {
                                return 65.0/458.0;
                            } else {
                                return 27.0/356.0;
                            }
                        } else {
                            if ( cl->stats.num_antecedents_rel <= 0.179437592626f ) {
                                return 89.0/402.0;
                            } else {
                                return 139.0/370.0;
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 13914.0f ) {
                            if ( cl->stats.num_overlap_literals <= 28.5f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.173198133707f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.115342825651f ) {
                                        return 145.0/386.0;
                                    } else {
                                        return 74.0/412.0;
                                    }
                                } else {
                                    return 139.0/374.0;
                                }
                            } else {
                                if ( cl->stats.sum_uip1_used <= 2.5f ) {
                                    return 164.0/198.0;
                                } else {
                                    return 110.0/236.0;
                                }
                            }
                        } else {
                            if ( cl->size() <= 11.5f ) {
                                return 118.0/392.0;
                            } else {
                                return 166.0/240.0;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.glue_rel_queue <= 0.786266565323f ) {
                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                            if ( cl->stats.num_overlap_literals <= 36.5f ) {
                                if ( cl->stats.glue_rel_queue <= 0.599274814129f ) {
                                    if ( cl->stats.dump_number <= 7.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.426692962646f ) {
                                            return 78.0/248.0;
                                        } else {
                                            return 57.0/304.0;
                                        }
                                    } else {
                                        return 106.0/248.0;
                                    }
                                } else {
                                    return 164.0/382.0;
                                }
                            } else {
                                return 214.0/374.0;
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals <= 23.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 12.5f ) {
                                    return 86.0/250.0;
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0879423767328f ) {
                                        return 204.0/388.0;
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 6.5f ) {
                                            return 175.0/230.0;
                                        } else {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.165635332465f ) {
                                                if ( cl->stats.sum_uip1_used <= 3.5f ) {
                                                    return 203.0/230.0;
                                                } else {
                                                    return 125.0/230.0;
                                                }
                                            } else {
                                                return 112.0/270.0;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.483310222626f ) {
                                    if ( rdb0_last_touched_diff <= 39596.0f ) {
                                        return 149.0/140.0;
                                    } else {
                                        return 201.0/112.0;
                                    }
                                } else {
                                    return 237.0/274.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 58.5f ) {
                            return 309.0/142.0;
                        } else {
                            return 214.0/282.0;
                        }
                    }
                }
            } else {
                if ( cl->stats.rdb1_last_touched_diff <= 10897.0f ) {
                    if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                        if ( cl->stats.sum_uip1_used <= 46.5f ) {
                            if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.0407369472086f ) {
                                    return 97.0/300.0;
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 67.5f ) {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0595202222466f ) {
                                                if ( cl->stats.num_antecedents_rel <= 0.112664833665f ) {
                                                    return 65.0/320.0;
                                                } else {
                                                    return 84.0/270.0;
                                                }
                                            } else {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.227282688022f ) {
                                                    if ( cl->stats.glue_rel_long <= 0.488865792751f ) {
                                                        return 44.0/340.0;
                                                    } else {
                                                        return 47.0/296.0;
                                                    }
                                                } else {
                                                    return 69.0/274.0;
                                                }
                                            }
                                        } else {
                                            if ( rdb0_last_touched_diff <= 11663.0f ) {
                                                return 39.0/444.0;
                                            } else {
                                                if ( cl->stats.rdb1_last_touched_diff <= 3809.5f ) {
                                                    return 86.0/440.0;
                                                } else {
                                                    if ( cl->stats.dump_number <= 6.5f ) {
                                                        return 33.0/436.0;
                                                    } else {
                                                        return 63.0/344.0;
                                                    }
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.dump_number <= 7.5f ) {
                                            return 62.0/334.0;
                                        } else {
                                            return 106.0/216.0;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_uip1_used <= 14.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.451613664627f ) {
                                        return 27.0/364.0;
                                    } else {
                                        return 117.0/572.0;
                                    }
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 1152015.5f ) {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 742353.0f ) {
                                            if ( cl->stats.num_overlap_literals <= 20.5f ) {
                                                if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                                    return 22.0/568.0;
                                                } else {
                                                    return 40.0/695.9;
                                                }
                                            } else {
                                                return 40.0/368.0;
                                            }
                                        } else {
                                            return 18.0/452.0;
                                        }
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 19.5f ) {
                                            return 45.0/460.0;
                                        } else {
                                            return 72.0/540.0;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 39.5f ) {
                                if ( cl->stats.glue_rel_queue <= 0.158436775208f ) {
                                    return 58.0/600.0;
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                                        if ( cl->stats.sum_uip1_used <= 93.5f ) {
                                            if ( rdb0_last_touched_diff <= 13375.0f ) {
                                                if ( cl->stats.num_antecedents_rel <= 0.144357949495f ) {
                                                    return 42.0/436.0;
                                                } else {
                                                    return 23.0/444.0;
                                                }
                                            } else {
                                                return 89.0/566.0;
                                            }
                                        } else {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 26332374.0f ) {
                                                if ( cl->stats.glue_rel_long <= 0.362306237221f ) {
                                                    return 41.0/438.0;
                                                } else {
                                                    return 14.0/723.9;
                                                }
                                            } else {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.0663086771965f ) {
                                                    return 44.0/388.0;
                                                } else {
                                                    if ( cl->stats.num_total_lits_antecedents <= 13.5f ) {
                                                        return 12.0/376.0;
                                                    } else {
                                                        return 35.0/562.0;
                                                    }
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0177587587386f ) {
                                            if ( cl->stats.size_rel <= 0.11396651715f ) {
                                                return 16.0/823.9;
                                            } else {
                                                return 24.0/382.0;
                                            }
                                        } else {
                                            if ( cl->stats.num_overlap_literals <= 8.5f ) {
                                                if ( cl->size() <= 7.5f ) {
                                                    if ( cl->stats.num_antecedents_rel <= 0.145334690809f ) {
                                                        if ( cl->stats.sum_delta_confl_uip1_used <= 30046472.0f ) {
                                                            return 20.0/671.9;
                                                        } else {
                                                            return 4.0/420.0;
                                                        }
                                                    } else {
                                                        if ( rdb0_last_touched_diff <= 2548.0f ) {
                                                            return 0.0/418.0;
                                                        } else {
                                                            return 2.0/432.0;
                                                        }
                                                    }
                                                } else {
                                                    return 1.0/701.9;
                                                }
                                            } else {
                                                if ( cl->stats.used_for_uip_creation <= 5.5f ) {
                                                    return 19.0/486.0;
                                                } else {
                                                    return 12.0/618.0;
                                                }
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_uip1_used <= 144.5f ) {
                                    if ( rdb0_last_touched_diff <= 11832.5f ) {
                                        if ( cl->stats.glue_rel_long <= 0.568608045578f ) {
                                            return 41.0/773.9;
                                        } else {
                                            return 35.0/380.0;
                                        }
                                    } else {
                                        return 74.0/414.0;
                                    }
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 5.5f ) {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.129382714629f ) {
                                            return 15.0/414.0;
                                        } else {
                                            return 30.0/356.0;
                                        }
                                    } else {
                                        return 15.0/648.0;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                            if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 31.5f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0736625045538f ) {
                                            return 31.0/322.0;
                                        } else {
                                            return 34.0/500.0;
                                        }
                                    } else {
                                        return 14.0/426.0;
                                    }
                                } else {
                                    return 65.0/586.0;
                                }
                            } else {
                                if ( cl->stats.sum_uip1_used <= 95.5f ) {
                                    if ( cl->stats.size_rel <= 0.341609954834f ) {
                                        if ( rdb0_last_touched_diff <= 3227.5f ) {
                                            if ( cl->stats.glue_rel_queue <= 0.402505755424f ) {
                                                return 15.0/442.0;
                                            } else {
                                                return 21.0/380.0;
                                            }
                                        } else {
                                            return 49.0/348.0;
                                        }
                                    } else {
                                        if ( cl->size() <= 15.5f ) {
                                            return 15.0/484.0;
                                        } else {
                                            return 25.0/394.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0606954060495f ) {
                                        return 19.0/372.0;
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 18.5f ) {
                                            return 5.0/388.0;
                                        } else {
                                            return 17.0/474.0;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 13.5f ) {
                                if ( rdb0_last_touched_diff <= 3148.5f ) {
                                    if ( cl->stats.dump_number <= 9.5f ) {
                                        return 7.0/492.0;
                                    } else {
                                        return 21.0/596.0;
                                    }
                                } else {
                                    return 41.0/490.0;
                                }
                            } else {
                                if ( cl->stats.sum_uip1_used <= 534.5f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 484117.5f ) {
                                        return 25.0/412.0;
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.166580826044f ) {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 20127758.0f ) {
                                                if ( cl->stats.num_overlap_literals <= 9.5f ) {
                                                    if ( cl->stats.sum_uip1_used <= 107.0f ) {
                                                        return 11.0/440.0;
                                                    } else {
                                                        return 1.0/681.9;
                                                    }
                                                } else {
                                                    return 15.0/380.0;
                                                }
                                            } else {
                                                return 26.0/498.0;
                                            }
                                        } else {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.219452440739f ) {
                                                return 3.0/685.9;
                                            } else {
                                                return 11.0/370.0;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0837218612432f ) {
                                        return 1.0/624.0;
                                    } else {
                                        return 6.0/416.0;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_overlap_literals_rel <= 0.130260765553f ) {
                        if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                            if ( rdb0_last_touched_diff <= 45672.0f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.181252807379f ) {
                                    if ( cl->stats.sum_uip1_used <= 85.5f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.0578685998917f ) {
                                            return 130.0/358.0;
                                        } else {
                                            if ( cl->size() <= 12.5f ) {
                                                if ( cl->stats.rdb1_last_touched_diff <= 20296.5f ) {
                                                    if ( cl->stats.num_antecedents_rel <= 0.157555669546f ) {
                                                        return 96.0/542.0;
                                                    } else {
                                                        return 41.0/372.0;
                                                    }
                                                } else {
                                                    if ( cl->stats.num_overlap_literals_rel <= 0.046524554491f ) {
                                                        return 89.0/286.0;
                                                    } else {
                                                        return 74.0/412.0;
                                                    }
                                                }
                                            } else {
                                                return 124.0/446.0;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 8.5f ) {
                                            if ( rdb0_last_touched_diff <= 27936.0f ) {
                                                return 40.0/546.0;
                                            } else {
                                                return 65.0/550.0;
                                            }
                                        } else {
                                            return 49.0/324.0;
                                        }
                                    }
                                } else {
                                    return 91.0/312.0;
                                }
                            } else {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.308431953192f ) {
                                    if ( cl->size() <= 12.5f ) {
                                        if ( rdb0_last_touched_diff <= 78026.5f ) {
                                            if ( cl->stats.sum_uip1_used <= 46.0f ) {
                                                return 115.0/326.0;
                                            } else {
                                                return 68.0/478.0;
                                            }
                                        } else {
                                            return 112.0/310.0;
                                        }
                                    } else {
                                        return 143.0/278.0;
                                    }
                                } else {
                                    return 200.0/356.0;
                                }
                            }
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 7.5f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 21810920.0f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 37045.5f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.0666777417064f ) {
                                            return 76.0/398.0;
                                        } else {
                                            if ( cl->size() <= 8.5f ) {
                                                if ( cl->stats.num_total_lits_antecedents <= 13.5f ) {
                                                    return 32.0/336.0;
                                                } else {
                                                    return 25.0/410.0;
                                                }
                                            } else {
                                                return 74.0/584.0;
                                            }
                                        }
                                    } else {
                                        return 66.0/306.0;
                                    }
                                } else {
                                    return 43.0/584.0;
                                }
                            } else {
                                if ( cl->stats.dump_number <= 13.5f ) {
                                    return 31.0/406.0;
                                } else {
                                    return 24.0/534.0;
                                }
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 50083.5f ) {
                            if ( cl->stats.glue_rel_long <= 0.496677100658f ) {
                                if ( cl->stats.glue_rel_queue <= 0.370224535465f ) {
                                    return 84.0/326.0;
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.450112760067f ) {
                                        return 52.0/304.0;
                                    } else {
                                        return 36.0/336.0;
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 6972.5f ) {
                                    return 66.0/508.0;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 14305.0f ) {
                                        return 98.0/214.0;
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 42.5f ) {
                                            if ( cl->stats.num_total_lits_antecedents <= 38.5f ) {
                                                return 73.0/256.0;
                                            } else {
                                                return 80.0/276.0;
                                            }
                                        } else {
                                            return 109.0/286.0;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 30.5f ) {
                                return 231.0/330.0;
                            } else {
                                return 98.0/280.0;
                            }
                        }
                    }
                }
            }
        } else {
            if ( rdb0_last_touched_diff <= 9957.0f ) {
                if ( cl->stats.antecedents_glue_long_reds_var <= 0.672951102257f ) {
                    if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                        if ( cl->size() <= 11.5f ) {
                            return 43.0/430.0;
                        } else {
                            if ( cl->size() <= 38.5f ) {
                                return 73.0/354.0;
                            } else {
                                return 46.0/346.0;
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 3688.5f ) {
                            if ( cl->stats.glue_rel_long <= 0.949708819389f ) {
                                return 22.0/456.0;
                            } else {
                                if ( cl->stats.glue_rel_queue <= 1.12205529213f ) {
                                    return 3.0/612.0;
                                } else {
                                    return 7.0/416.0;
                                }
                            }
                        } else {
                            return 29.0/374.0;
                        }
                    }
                } else {
                    if ( rdb0_last_touched_diff <= 796.5f ) {
                        return 34.0/432.0;
                    } else {
                        if ( cl->stats.num_overlap_literals_rel <= 0.302496492863f ) {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                return 101.0/428.0;
                            } else {
                                return 65.0/546.0;
                            }
                        } else {
                            if ( cl->stats.dump_number <= 5.5f ) {
                                return 153.0/310.0;
                            } else {
                                return 91.0/548.0;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.num_overlap_literals_rel <= 0.599561691284f ) {
                    if ( cl->stats.antec_num_total_lits_rel <= 0.238689720631f ) {
                        if ( cl->stats.glue <= 8.5f ) {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 1669325.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 16507.0f ) {
                                    return 127.0/406.0;
                                } else {
                                    return 126.0/230.0;
                                }
                            } else {
                                return 58.0/354.0;
                            }
                        } else {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 179.0f ) {
                                return 291.0/132.0;
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.964437842369f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 15231.5f ) {
                                        return 93.0/236.0;
                                    } else {
                                        return 138.0/180.0;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 12.5f ) {
                                        return 55.0/292.0;
                                    } else {
                                        return 118.0/282.0;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 58788.5f ) {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                if ( cl->stats.dump_number <= 4.5f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 139.0f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 49.5f ) {
                                            return 129.0/142.0;
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 180.5f ) {
                                                if ( cl->stats.num_overlap_literals <= 44.5f ) {
                                                    return 241.0/58.0;
                                                } else {
                                                    return 178.0/112.0;
                                                }
                                            } else {
                                                return 194.0/14.0;
                                            }
                                        }
                                    } else {
                                        return 163.0/260.0;
                                    }
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 1029969.5f ) {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 4.6533331871f ) {
                                            return 151.0/340.0;
                                        } else {
                                            return 163.0/162.0;
                                        }
                                    } else {
                                        return 151.0/428.0;
                                    }
                                }
                            } else {
                                return 158.0/462.0;
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 1.1142950058f ) {
                                return 314.0/210.0;
                            } else {
                                return 185.0/50.0;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.glue <= 12.5f ) {
                        if ( cl->stats.size_rel <= 1.17650818825f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 11339.5f ) {
                                    return 123.0/294.0;
                                } else {
                                    return 154.0/142.0;
                                }
                            } else {
                                return 281.0/280.0;
                            }
                        } else {
                            return 215.0/102.0;
                        }
                    } else {
                        if ( cl->stats.num_overlap_literals <= 518.5f ) {
                            if ( cl->stats.glue_rel_queue <= 1.04358756542f ) {
                                if ( cl->stats.dump_number <= 5.5f ) {
                                    return 219.0/126.0;
                                } else {
                                    return 145.0/152.0;
                                }
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.530122935772f ) {
                                    return 159.0/94.0;
                                } else {
                                    if ( rdb0_last_touched_diff <= 53891.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 7746.0f ) {
                                            return 190.0/50.0;
                                        } else {
                                            return 361.0/68.0;
                                        }
                                    } else {
                                        return 198.0/8.0;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.glue <= 41.5f ) {
                                return 373.0/40.0;
                            } else {
                                return 276.0/62.0;
                            }
                        }
                    }
                }
            }
        }
    } else {
        if ( cl->stats.glue_rel_long <= 0.752692818642f ) {
            if ( cl->stats.sum_uip1_used <= 5.5f ) {
                if ( cl->stats.glue <= 4.5f ) {
                    if ( rdb0_last_touched_diff <= 38133.5f ) {
                        if ( cl->stats.num_antecedents_rel <= 0.149183809757f ) {
                            return 97.0/246.0;
                        } else {
                            return 78.0/306.0;
                        }
                    } else {
                        if ( cl->stats.size_rel <= 0.145045116544f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 14.5f ) {
                                return 133.0/152.0;
                            } else {
                                return 135.0/282.0;
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 171548.5f ) {
                                if ( cl->stats.size_rel <= 0.301984369755f ) {
                                    return 182.0/232.0;
                                } else {
                                    return 205.0/160.0;
                                }
                            } else {
                                return 194.0/78.0;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.sum_delta_confl_uip1_used <= 13.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 53988.5f ) {
                            if ( cl->stats.num_antecedents_rel <= 0.21399076283f ) {
                                return 182.0/216.0;
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.615481734276f ) {
                                    return 136.0/140.0;
                                } else {
                                    return 172.0/96.0;
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.648281216621f ) {
                                if ( cl->stats.size_rel <= 0.541394531727f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.215720891953f ) {
                                        if ( cl->size() <= 10.5f ) {
                                            return 183.0/146.0;
                                        } else {
                                            return 202.0/80.0;
                                        }
                                    } else {
                                        return 136.0/130.0;
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 137431.0f ) {
                                        return 175.0/72.0;
                                    } else {
                                        return 328.0/74.0;
                                    }
                                }
                            } else {
                                if ( cl->size() <= 13.5f ) {
                                    return 176.0/88.0;
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.211334139109f ) {
                                        return 183.0/46.0;
                                    } else {
                                        return 349.0/58.0;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.dump_number <= 10.5f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.924444437027f ) {
                                if ( cl->stats.num_overlap_literals <= 8.5f ) {
                                    return 127.0/338.0;
                                } else {
                                    if ( rdb0_last_touched_diff <= 37274.5f ) {
                                        return 104.0/216.0;
                                    } else {
                                        return 137.0/182.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.257373332977f ) {
                                    return 137.0/198.0;
                                } else {
                                    return 193.0/160.0;
                                }
                            }
                        } else {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.446684420109f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 142142.5f ) {
                                    if ( cl->stats.size_rel <= 0.543649375439f ) {
                                        return 272.0/212.0;
                                    } else {
                                        return 131.0/158.0;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.675286412239f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.134628489614f ) {
                                            return 232.0/188.0;
                                        } else {
                                            if ( cl->stats.num_overlap_literals <= 10.5f ) {
                                                return 154.0/96.0;
                                            } else {
                                                return 316.0/136.0;
                                            }
                                        }
                                    } else {
                                        return 135.0/122.0;
                                    }
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                    return 160.0/90.0;
                                } else {
                                    return 204.0/72.0;
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.dump_number <= 21.5f ) {
                    if ( cl->stats.sum_delta_confl_uip1_used <= 710916.0f ) {
                        if ( rdb0_last_touched_diff <= 41272.0f ) {
                            if ( cl->stats.dump_number <= 9.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 8803.0f ) {
                                    return 42.0/490.0;
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.624685287476f ) {
                                        if ( cl->stats.size_rel <= 0.14045009017f ) {
                                            return 25.0/374.0;
                                        } else {
                                            if ( cl->stats.glue <= 5.5f ) {
                                                return 83.0/440.0;
                                            } else {
                                                return 58.0/498.0;
                                            }
                                        }
                                    } else {
                                        return 65.0/264.0;
                                    }
                                }
                            } else {
                                return 101.0/272.0;
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals <= 9.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0217935964465f ) {
                                    return 116.0/194.0;
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.456752836704f ) {
                                        return 96.0/506.0;
                                    } else {
                                        return 175.0/370.0;
                                    }
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.484144330025f ) {
                                        return 111.0/186.0;
                                    } else {
                                        return 252.0/276.0;
                                    }
                                } else {
                                    if ( cl->stats.glue <= 7.5f ) {
                                        return 94.0/260.0;
                                    } else {
                                        return 123.0/204.0;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.sum_uip1_used <= 33.5f ) {
                            if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                                return 134.0/490.0;
                            } else {
                                return 38.0/348.0;
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 37529.5f ) {
                                if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                                    return 14.0/715.9;
                                } else {
                                    return 51.0/532.0;
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 63867.0f ) {
                                    return 46.0/412.0;
                                } else {
                                    return 64.0/378.0;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_last_touched_diff <= 204057.5f ) {
                        if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 15828387.0f ) {
                                if ( cl->stats.sum_uip1_used <= 23.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 57.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0687365084887f ) {
                                            return 286.0/250.0;
                                        } else {
                                            return 258.0/322.0;
                                        }
                                    } else {
                                        return 123.0/196.0;
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.130279690027f ) {
                                        return 178.0/286.0;
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.492384850979f ) {
                                            return 152.0/322.0;
                                        } else {
                                            return 121.0/418.0;
                                        }
                                    }
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                    return 83.0/438.0;
                                } else {
                                    return 89.0/260.0;
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 3533.5f ) {
                                return 58.0/384.0;
                            } else {
                                return 89.0/294.0;
                            }
                        }
                    } else {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 4676358.0f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 492305.0f ) {
                                if ( rdb0_last_touched_diff <= 274751.0f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.0546875f ) {
                                        return 139.0/194.0;
                                    } else {
                                        return 167.0/126.0;
                                    }
                                } else {
                                    if ( cl->stats.glue <= 6.5f ) {
                                        return 205.0/152.0;
                                    } else {
                                        return 231.0/114.0;
                                    }
                                }
                            } else {
                                return 185.0/244.0;
                            }
                        } else {
                            if ( cl->stats.glue <= 7.5f ) {
                                return 197.0/406.0;
                            } else {
                                return 135.0/180.0;
                            }
                        }
                    }
                }
            }
        } else {
            if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                if ( cl->stats.sum_uip1_used <= 14.5f ) {
                    if ( rdb0_last_touched_diff <= 2045.0f ) {
                        return 66.0/270.0;
                    } else {
                        return 216.0/410.0;
                    }
                } else {
                    if ( cl->stats.dump_number <= 13.5f ) {
                        return 23.0/398.0;
                    } else {
                        return 55.0/312.0;
                    }
                }
            } else {
                if ( cl->stats.glue_rel_long <= 1.01011371613f ) {
                    if ( cl->stats.num_total_lits_antecedents <= 50.5f ) {
                        if ( cl->stats.sum_uip1_used <= 5.5f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 19.5f ) {
                                return 147.0/202.0;
                            } else {
                                if ( rdb0_last_touched_diff <= 77689.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 32.5f ) {
                                        return 144.0/182.0;
                                    } else {
                                        return 239.0/174.0;
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 0.600408017635f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.110249996185f ) {
                                            return 186.0/68.0;
                                        } else {
                                            return 143.0/118.0;
                                        }
                                    } else {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 109.5f ) {
                                            if ( cl->stats.num_overlap_literals <= 13.5f ) {
                                                return 200.0/22.0;
                                            } else {
                                                return 188.0/38.0;
                                            }
                                        } else {
                                            return 166.0/76.0;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 98591.5f ) {
                                if ( rdb0_last_touched_diff <= 67125.0f ) {
                                    return 103.0/394.0;
                                } else {
                                    return 109.0/216.0;
                                }
                            } else {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.204861104488f ) {
                                    return 141.0/176.0;
                                } else {
                                    return 188.0/154.0;
                                }
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 56597.5f ) {
                            if ( cl->stats.num_overlap_literals <= 36.5f ) {
                                return 210.0/324.0;
                            } else {
                                if ( cl->stats.size_rel <= 0.540885269642f ) {
                                    return 215.0/188.0;
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 1.5f ) {
                                        return 253.0/104.0;
                                    } else {
                                        return 201.0/182.0;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 219734.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.900979399681f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 5866.0f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.702170431614f ) {
                                            if ( cl->stats.num_total_lits_antecedents <= 234.0f ) {
                                                if ( cl->stats.num_overlap_literals <= 75.5f ) {
                                                    if ( cl->stats.size_rel <= 0.618177890778f ) {
                                                        return 180.0/70.0;
                                                    } else {
                                                        if ( cl->stats.glue_rel_long <= 0.926803588867f ) {
                                                            return 307.0/88.0;
                                                        } else {
                                                            return 192.0/20.0;
                                                        }
                                                    }
                                                } else {
                                                    return 172.0/84.0;
                                                }
                                            } else {
                                                return 228.0/40.0;
                                            }
                                        } else {
                                            return 174.0/100.0;
                                        }
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.267508685589f ) {
                                            return 151.0/98.0;
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 0.906686902046f ) {
                                                return 231.0/322.0;
                                            } else {
                                                return 199.0/142.0;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 0.469392091036f ) {
                                        return 186.0/92.0;
                                    } else {
                                        if ( cl->stats.glue <= 9.5f ) {
                                            return 192.0/74.0;
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 133949.5f ) {
                                                return 289.0/72.0;
                                            } else {
                                                return 222.0/24.0;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.396371304989f ) {
                                    return 282.0/98.0;
                                } else {
                                    if ( cl->size() <= 16.5f ) {
                                        return 187.0/64.0;
                                    } else {
                                        if ( cl->size() <= 31.5f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.806049823761f ) {
                                                return 259.0/22.0;
                                            } else {
                                                return 192.0/30.0;
                                            }
                                        } else {
                                            if ( rdb0_last_touched_diff <= 330372.0f ) {
                                                return 194.0/50.0;
                                            } else {
                                                return 204.0/32.0;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_overlap_literals <= 22.5f ) {
                        if ( rdb0_last_touched_diff <= 86068.5f ) {
                            if ( cl->stats.sum_uip1_used <= 4.5f ) {
                                if ( cl->size() <= 20.5f ) {
                                    return 182.0/158.0;
                                } else {
                                    return 248.0/114.0;
                                }
                            } else {
                                return 117.0/314.0;
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 5.5f ) {
                                if ( cl->size() <= 15.5f ) {
                                    return 164.0/100.0;
                                } else {
                                    if ( cl->stats.glue <= 19.5f ) {
                                        return 402.0/32.0;
                                    } else {
                                        return 200.0/70.0;
                                    }
                                }
                            } else {
                                return 161.0/204.0;
                            }
                        }
                    } else {
                        if ( cl->stats.glue <= 8.5f ) {
                            return 304.0/194.0;
                        } else {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 2636.0f ) {
                                if ( cl->size() <= 12.5f ) {
                                    return 183.0/62.0;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 117551.5f ) {
                                        if ( cl->stats.num_overlap_literals <= 112.5f ) {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.901234567165f ) {
                                                return 159.0/84.0;
                                            } else {
                                                if ( cl->stats.num_antecedents_rel <= 0.51615011692f ) {
                                                    if ( cl->stats.size_rel <= 1.11545205116f ) {
                                                        if ( cl->stats.glue_rel_long <= 1.22468590736f ) {
                                                            return 185.0/50.0;
                                                        } else {
                                                            return 174.0/66.0;
                                                        }
                                                    } else {
                                                        return 274.0/40.0;
                                                    }
                                                } else {
                                                    if ( cl->stats.num_antecedents_rel <= 0.786175072193f ) {
                                                        return 330.0/26.0;
                                                    } else {
                                                        return 335.0/80.0;
                                                    }
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 1.63841414452f ) {
                                                if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                                    return 250.0/10.0;
                                                } else {
                                                    if ( cl->stats.num_total_lits_antecedents <= 1610.0f ) {
                                                        if ( cl->stats.sum_uip1_used <= 1.5f ) {
                                                            if ( cl->stats.num_overlap_literals_rel <= 1.8304759264f ) {
                                                                if ( rdb0_last_touched_diff <= 55029.0f ) {
                                                                    return 266.0/60.0;
                                                                } else {
                                                                    if ( cl->stats.num_overlap_literals_rel <= 0.853200078011f ) {
                                                                        return 192.0/20.0;
                                                                    } else {
                                                                        return 307.0/12.0;
                                                                    }
                                                                }
                                                            } else {
                                                                if ( cl->stats.num_antecedents_rel <= 1.86564171314f ) {
                                                                    return 203.0/68.0;
                                                                } else {
                                                                    return 243.0/34.0;
                                                                }
                                                            }
                                                        } else {
                                                            return 297.0/86.0;
                                                        }
                                                    } else {
                                                        return 277.0/18.0;
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.num_antecedents_rel <= 0.748526513577f ) {
                                                    return 207.0/18.0;
                                                } else {
                                                    return 355.0/12.0;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 598507.0f ) {
                                            if ( cl->stats.glue_rel_queue <= 1.19670963287f ) {
                                                if ( cl->stats.rdb1_act_ranking_top_10 <= 6.5f ) {
                                                    if ( cl->stats.glue <= 23.5f ) {
                                                        if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                                            return 281.0/4.0;
                                                        } else {
                                                            return 382.0/38.0;
                                                        }
                                                    } else {
                                                        return 306.0/2.0;
                                                    }
                                                } else {
                                                    if ( cl->stats.size_rel <= 1.30788469315f ) {
                                                        if ( cl->size() <= 26.5f ) {
                                                            return 195.0/52.0;
                                                        } else {
                                                            return 205.0/38.0;
                                                        }
                                                    } else {
                                                        return 370.0/22.0;
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.num_total_lits_antecedents <= 558.5f ) {
                                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 9.5f ) {
                                                        if ( cl->stats.rdb1_act_ranking_top_10 <= 6.5f ) {
                                                            if ( cl->stats.size_rel <= 1.00053286552f ) {
                                                                return 226.0/16.0;
                                                            } else {
                                                                if ( cl->stats.antecedents_glue_long_reds_var <= 16.9819831848f ) {
                                                                    return 206.0/10.0;
                                                                } else {
                                                                    return 252.0/2.0;
                                                                }
                                                            }
                                                        } else {
                                                            if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                                                if ( cl->stats.size_rel <= 1.31530952454f ) {
                                                                    return 200.0/38.0;
                                                                } else {
                                                                    return 191.0/16.0;
                                                                }
                                                            } else {
                                                                return 341.0/16.0;
                                                            }
                                                        }
                                                    } else {
                                                        return 293.0/2.0;
                                                    }
                                                } else {
                                                    if ( cl->stats.antec_num_total_lits_rel <= 3.14947080612f ) {
                                                        return 1;
                                                    } else {
                                                        return 208.0/4.0;
                                                    }
                                                }
                                            }
                                        } else {
                                            return 291.0/54.0;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 72964.0f ) {
                                    if ( cl->stats.num_overlap_literals <= 135.5f ) {
                                        return 209.0/328.0;
                                    } else {
                                        return 151.0/90.0;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 8.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.394515693188f ) {
                                            return 186.0/40.0;
                                        } else {
                                            if ( cl->stats.size_rel <= 1.11199331284f ) {
                                                return 152.0/106.0;
                                            } else {
                                                return 205.0/94.0;
                                            }
                                        }
                                    } else {
                                        return 159.0/146.0;
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

static double estimator_should_keep_long_conf4_cluster0_4(
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
        if ( cl->stats.antec_num_total_lits_rel <= 0.448130786419f ) {
            if ( cl->stats.glue_rel_queue <= 0.788687944412f ) {
                if ( cl->stats.sum_delta_confl_uip1_used <= 2540.5f ) {
                    if ( rdb0_last_touched_diff <= 67345.0f ) {
                        if ( cl->stats.dump_number <= 2.5f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 33.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 13268.5f ) {
                                    return 105.0/286.0;
                                } else {
                                    return 84.0/360.0;
                                }
                            } else {
                                return 234.0/254.0;
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 4.22353744507f ) {
                                if ( cl->stats.num_overlap_literals <= 6.5f ) {
                                    if ( cl->stats.size_rel <= 0.249457657337f ) {
                                        return 103.0/244.0;
                                    } else {
                                        return 101.0/226.0;
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.294104456902f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 28.5f ) {
                                            return 148.0/274.0;
                                        } else {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.145384132862f ) {
                                                return 137.0/134.0;
                                            } else {
                                                return 136.0/130.0;
                                            }
                                        }
                                    } else {
                                        return 131.0/280.0;
                                    }
                                }
                            } else {
                                return 142.0/114.0;
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 293530.5f ) {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 16.5f ) {
                                if ( cl->stats.glue_rel_queue <= 0.334316641092f ) {
                                    return 159.0/214.0;
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                        return 159.0/144.0;
                                    } else {
                                        if ( cl->stats.size_rel <= 0.564238250256f ) {
                                            if ( cl->stats.rdb1_act_ranking_top_10 <= 5.5f ) {
                                                return 190.0/130.0;
                                            } else {
                                                return 331.0/154.0;
                                            }
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 41.5f ) {
                                                return 176.0/58.0;
                                            } else {
                                                return 233.0/54.0;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.size_rel <= 0.204897910357f ) {
                                    return 109.0/212.0;
                                } else {
                                    if ( rdb0_last_touched_diff <= 119539.5f ) {
                                        return 230.0/298.0;
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.211872965097f ) {
                                            return 164.0/102.0;
                                        } else {
                                            return 143.0/134.0;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.dump_number <= 59.5f ) {
                                if ( cl->stats.glue <= 6.5f ) {
                                    return 197.0/74.0;
                                } else {
                                    return 255.0/22.0;
                                }
                            } else {
                                return 186.0/112.0;
                            }
                        }
                    }
                } else {
                    if ( rdb0_last_touched_diff <= 48499.0f ) {
                        if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                            if ( cl->stats.sum_uip1_used <= 40.5f ) {
                                if ( cl->stats.size_rel <= 0.525147914886f ) {
                                    if ( rdb0_last_touched_diff <= 35082.0f ) {
                                        if ( cl->stats.num_overlap_literals <= 20.5f ) {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 18051.5f ) {
                                                return 51.0/338.0;
                                            } else {
                                                if ( cl->stats.glue_rel_queue <= 0.516542553902f ) {
                                                    if ( cl->stats.size_rel <= 0.23866635561f ) {
                                                        if ( cl->size() <= 5.5f ) {
                                                            return 104.0/410.0;
                                                        } else {
                                                            return 96.0/298.0;
                                                        }
                                                    } else {
                                                        if ( rdb0_last_touched_diff <= 26277.5f ) {
                                                            return 72.0/268.0;
                                                        } else {
                                                            return 35.0/322.0;
                                                        }
                                                    }
                                                } else {
                                                    if ( cl->stats.sum_delta_confl_uip1_used <= 583766.5f ) {
                                                        return 154.0/362.0;
                                                    } else {
                                                        return 66.0/282.0;
                                                    }
                                                }
                                            }
                                        } else {
                                            return 169.0/462.0;
                                        }
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.200611203909f ) {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 258508.5f ) {
                                                return 115.0/446.0;
                                            } else {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.075235620141f ) {
                                                    return 102.0/226.0;
                                                } else {
                                                    return 75.0/244.0;
                                                }
                                            }
                                        } else {
                                            return 159.0/368.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue <= 9.5f ) {
                                        if ( cl->stats.dump_number <= 13.5f ) {
                                            if ( rdb0_last_touched_diff <= 30944.5f ) {
                                                return 111.0/244.0;
                                            } else {
                                                return 79.0/254.0;
                                            }
                                        } else {
                                            return 198.0/284.0;
                                        }
                                    } else {
                                        return 138.0/432.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_uip1_used <= 77.5f ) {
                                    if ( rdb0_last_touched_diff <= 24406.0f ) {
                                        return 53.0/348.0;
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.077095463872f ) {
                                            return 75.0/244.0;
                                        } else {
                                            if ( rdb0_last_touched_diff <= 34133.5f ) {
                                                return 59.0/322.0;
                                            } else {
                                                return 63.0/302.0;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 292.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0404115244746f ) {
                                            if ( cl->stats.num_total_lits_antecedents <= 15.5f ) {
                                                return 42.0/410.0;
                                            } else {
                                                return 70.0/280.0;
                                            }
                                        } else {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.133146762848f ) {
                                                return 29.0/468.0;
                                            } else {
                                                return 61.0/518.0;
                                            }
                                        }
                                    } else {
                                        return 30.0/626.0;
                                    }
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 3284.0f ) {
                                if ( cl->stats.sum_uip1_used <= 19.5f ) {
                                    if ( cl->stats.dump_number <= 14.5f ) {
                                        if ( cl->stats.sum_uip1_used <= 5.5f ) {
                                            return 77.0/278.0;
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 21630.0f ) {
                                                return 28.0/400.0;
                                            } else {
                                                return 55.0/366.0;
                                            }
                                        }
                                    } else {
                                        return 91.0/208.0;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 13455.5f ) {
                                        return 29.0/749.9;
                                    } else {
                                        if ( rdb0_last_touched_diff <= 426.5f ) {
                                            return 25.0/703.9;
                                        } else {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 16315113.0f ) {
                                                if ( cl->size() <= 9.5f ) {
                                                    return 62.0/701.9;
                                                } else {
                                                    return 69.0/476.0;
                                                }
                                            } else {
                                                return 29.0/572.0;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 4.5f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 24767304.0f ) {
                                        if ( cl->stats.dump_number <= 32.5f ) {
                                            if ( rdb0_last_touched_diff <= 7430.0f ) {
                                                if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                                                    if ( cl->stats.sum_delta_confl_uip1_used <= 444957.5f ) {
                                                        return 83.0/282.0;
                                                    } else {
                                                        return 46.0/332.0;
                                                    }
                                                } else {
                                                    if ( cl->stats.antec_num_total_lits_rel <= 0.182984918356f ) {
                                                        if ( cl->stats.num_overlap_literals_rel <= 0.038307748735f ) {
                                                            return 43.0/494.0;
                                                        } else {
                                                            return 31.0/500.0;
                                                        }
                                                    } else {
                                                        return 53.0/350.0;
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.sum_uip1_used <= 22.5f ) {
                                                    return 119.0/358.0;
                                                } else {
                                                    return 41.0/326.0;
                                                }
                                            }
                                        } else {
                                            return 91.0/294.0;
                                        }
                                    } else {
                                        return 26.0/482.0;
                                    }
                                } else {
                                    return 149.0/482.0;
                                }
                            }
                        }
                    } else {
                        if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 1159397.0f ) {
                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.542221188545f ) {
                                        return 95.0/224.0;
                                    } else {
                                        return 86.0/252.0;
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 29.5f ) {
                                        if ( cl->stats.num_overlap_literals <= 4.5f ) {
                                            return 91.0/250.0;
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 20.5f ) {
                                                return 116.0/164.0;
                                            } else {
                                                return 113.0/216.0;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 1.36934232712f ) {
                                            return 256.0/200.0;
                                        } else {
                                            return 149.0/150.0;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->size() <= 17.5f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 75931184.0f ) {
                                        if ( cl->stats.sum_uip1_used <= 25.5f ) {
                                            return 136.0/254.0;
                                        } else {
                                            if ( rdb0_last_touched_diff <= 92903.0f ) {
                                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.204861104488f ) {
                                                    if ( cl->stats.num_overlap_literals <= 5.5f ) {
                                                        return 83.0/358.0;
                                                    } else {
                                                        return 95.0/228.0;
                                                    }
                                                } else {
                                                    return 46.0/328.0;
                                                }
                                            } else {
                                                return 88.0/256.0;
                                            }
                                        }
                                    } else {
                                        return 56.0/368.0;
                                    }
                                } else {
                                    return 178.0/238.0;
                                }
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.478298604488f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 14.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0296573676169f ) {
                                        return 151.0/206.0;
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.242646485567f ) {
                                            return 61.0/254.0;
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 89539.0f ) {
                                                return 68.0/262.0;
                                            } else {
                                                if ( cl->stats.num_antecedents_rel <= 0.127562820911f ) {
                                                    return 95.0/274.0;
                                                } else {
                                                    return 118.0/204.0;
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0345387160778f ) {
                                        return 170.0/194.0;
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 140969.5f ) {
                                            if ( cl->stats.sum_uip1_used <= 41.5f ) {
                                                if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                                    if ( cl->stats.num_overlap_literals <= 9.5f ) {
                                                        return 149.0/298.0;
                                                    } else {
                                                        return 203.0/250.0;
                                                    }
                                                } else {
                                                    if ( cl->stats.num_total_lits_antecedents <= 26.5f ) {
                                                        return 117.0/314.0;
                                                    } else {
                                                        return 175.0/370.0;
                                                    }
                                                }
                                            } else {
                                                return 125.0/514.0;
                                            }
                                        } else {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 5124215.0f ) {
                                                if ( cl->stats.dump_number <= 34.5f ) {
                                                    if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                                        return 147.0/136.0;
                                                    } else {
                                                        return 196.0/352.0;
                                                    }
                                                } else {
                                                    if ( rdb0_last_touched_diff <= 473722.0f ) {
                                                        if ( rdb0_last_touched_diff <= 276210.0f ) {
                                                            return 223.0/142.0;
                                                        } else {
                                                            return 223.0/98.0;
                                                        }
                                                    } else {
                                                        return 169.0/158.0;
                                                    }
                                                }
                                            } else {
                                                return 178.0/336.0;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 574936.0f ) {
                                    if ( cl->stats.size_rel <= 0.646224915981f ) {
                                        if ( cl->stats.sum_uip1_used <= 4.5f ) {
                                            return 201.0/124.0;
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 135470.0f ) {
                                                return 142.0/236.0;
                                            } else {
                                                return 197.0/208.0;
                                            }
                                        }
                                    } else {
                                        return 270.0/164.0;
                                    }
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 21.5f ) {
                                        return 154.0/160.0;
                                    } else {
                                        if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                            return 103.0/256.0;
                                        } else {
                                            return 113.0/188.0;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                    if ( cl->stats.sum_delta_confl_uip1_used <= 273.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 47505.0f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.299382716417f ) {
                                if ( cl->stats.glue_rel_queue <= 0.954338550568f ) {
                                    return 106.0/188.0;
                                } else {
                                    return 172.0/166.0;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 66.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 40.5f ) {
                                        return 146.0/122.0;
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 34.5f ) {
                                            return 272.0/98.0;
                                        } else {
                                            return 170.0/84.0;
                                        }
                                    }
                                } else {
                                    return 276.0/50.0;
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 97549.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.0355029590428f ) {
                                    return 209.0/148.0;
                                } else {
                                    if ( cl->stats.size_rel <= 0.868113636971f ) {
                                        if ( cl->stats.glue_rel_queue <= 1.09349870682f ) {
                                            return 243.0/116.0;
                                        } else {
                                            return 179.0/30.0;
                                        }
                                    } else {
                                        return 256.0/28.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 25.5f ) {
                                    return 176.0/132.0;
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.51530611515f ) {
                                        if ( cl->stats.size_rel <= 0.873469352722f ) {
                                            return 287.0/128.0;
                                        } else {
                                            return 194.0/22.0;
                                        }
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 60.5f ) {
                                            if ( cl->stats.glue_rel_long <= 0.93307107687f ) {
                                                return 228.0/78.0;
                                            } else {
                                                return 338.0/56.0;
                                            }
                                        } else {
                                            if ( cl->stats.dump_number <= 28.5f ) {
                                                if ( cl->stats.glue_rel_queue <= 1.01160538197f ) {
                                                    return 247.0/26.0;
                                                } else {
                                                    return 366.0/12.0;
                                                }
                                            } else {
                                                return 223.0/34.0;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.sum_uip1_used <= 18.5f ) {
                            if ( cl->stats.size_rel <= 0.373904109001f ) {
                                if ( rdb0_last_touched_diff <= 36866.0f ) {
                                    return 105.0/292.0;
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 35.5f ) {
                                        return 196.0/298.0;
                                    } else {
                                        return 261.0/178.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 68169.5f ) {
                                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                        return 135.0/278.0;
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 113.5f ) {
                                            if ( cl->stats.num_antecedents_rel <= 0.18126142025f ) {
                                                return 199.0/366.0;
                                            } else {
                                                if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                                    return 182.0/170.0;
                                                } else {
                                                    return 115.0/146.0;
                                                }
                                            }
                                        } else {
                                            return 161.0/122.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 26.5f ) {
                                        if ( cl->stats.size_rel <= 0.877018272877f ) {
                                            if ( cl->size() <= 17.5f ) {
                                                return 124.0/168.0;
                                            } else {
                                                return 218.0/162.0;
                                            }
                                        } else {
                                            return 184.0/84.0;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.857890963554f ) {
                                            return 188.0/88.0;
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 1.00091075897f ) {
                                                return 192.0/44.0;
                                            } else {
                                                return 193.0/76.0;
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 19775740.0f ) {
                                if ( cl->stats.dump_number <= 26.5f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.123468205333f ) {
                                        return 111.0/526.0;
                                    } else {
                                        return 108.0/256.0;
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 151604.5f ) {
                                        return 175.0/320.0;
                                    } else {
                                        return 135.0/172.0;
                                    }
                                }
                            } else {
                                return 84.0/376.0;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_total_lits_antecedents <= 28.5f ) {
                        return 80.0/528.0;
                    } else {
                        if ( cl->stats.sum_uip1_used <= 9.5f ) {
                            return 166.0/342.0;
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.92625105381f ) {
                                return 63.0/266.0;
                            } else {
                                return 55.0/426.0;
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.glue <= 10.5f ) {
                if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                    if ( cl->stats.glue <= 6.5f ) {
                        if ( cl->stats.size_rel <= 0.516052365303f ) {
                            if ( cl->stats.num_overlap_literals <= 105.5f ) {
                                return 166.0/454.0;
                            } else {
                                return 157.0/282.0;
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 68313.0f ) {
                                return 234.0/316.0;
                            } else {
                                return 294.0/140.0;
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 77870.0f ) {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 285.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 69.5f ) {
                                    return 148.0/132.0;
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.719051241875f ) {
                                        return 147.0/120.0;
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.863214612007f ) {
                                            return 166.0/70.0;
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 1.00125312805f ) {
                                                return 191.0/52.0;
                                            } else {
                                                return 242.0/48.0;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.621972322464f ) {
                                    return 241.0/310.0;
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 140.5f ) {
                                        return 89.0/290.0;
                                    } else {
                                        return 188.0/318.0;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 312003.0f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 189.0f ) {
                                    if ( cl->size() <= 24.5f ) {
                                        if ( cl->stats.size_rel <= 0.623451828957f ) {
                                            return 268.0/84.0;
                                        } else {
                                            if ( cl->stats.num_overlap_literals <= 46.5f ) {
                                                return 229.0/64.0;
                                            } else {
                                                return 234.0/28.0;
                                            }
                                        }
                                    } else {
                                        return 337.0/136.0;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 4.5f ) {
                                        return 206.0/232.0;
                                    } else {
                                        if ( cl->stats.sum_uip1_used <= 6.5f ) {
                                            return 196.0/98.0;
                                        } else {
                                            return 138.0/140.0;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_uip1_used <= 1.5f ) {
                                    return 292.0/36.0;
                                } else {
                                    return 204.0/74.0;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.sum_delta_confl_uip1_used <= 995296.0f ) {
                        return 166.0/434.0;
                    } else {
                        return 45.0/352.0;
                    }
                }
            } else {
                if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                    if ( rdb0_last_touched_diff <= 99898.5f ) {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 15291.5f ) {
                            if ( cl->stats.glue_rel_long <= 0.992657423019f ) {
                                if ( cl->stats.glue_rel_queue <= 0.954473137856f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 40241.5f ) {
                                        if ( cl->stats.glue_rel_long <= 0.832534313202f ) {
                                            return 160.0/118.0;
                                        } else {
                                            return 175.0/108.0;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 61796.5f ) {
                                            return 199.0/52.0;
                                        } else {
                                            return 161.0/92.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 287.0f ) {
                                        return 215.0/100.0;
                                    } else {
                                        return 223.0/44.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 48.5f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.42995017767f ) {
                                        return 235.0/44.0;
                                    } else {
                                        return 215.0/92.0;
                                    }
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 1.5f ) {
                                        if ( cl->stats.glue_rel_long <= 1.38080668449f ) {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 0.5f ) {
                                                return 217.0/60.0;
                                            } else {
                                                if ( cl->stats.antec_num_total_lits_rel <= 2.5970621109f ) {
                                                    if ( cl->stats.num_antecedents_rel <= 1.71679067612f ) {
                                                        if ( cl->stats.num_total_lits_antecedents <= 681.5f ) {
                                                            if ( cl->stats.dump_number <= 3.5f ) {
                                                                return 334.0/64.0;
                                                            } else {
                                                                if ( cl->stats.rdb1_last_touched_diff <= 51084.0f ) {
                                                                    return 272.0/12.0;
                                                                } else {
                                                                    if ( cl->stats.glue_rel_queue <= 1.13254857063f ) {
                                                                        return 194.0/26.0;
                                                                    } else {
                                                                        return 206.0/34.0;
                                                                    }
                                                                }
                                                            }
                                                        } else {
                                                            return 234.0/8.0;
                                                        }
                                                    } else {
                                                        return 199.0/50.0;
                                                    }
                                                } else {
                                                    return 284.0/14.0;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 1.44822740555f ) {
                                                return 209.0/2.0;
                                            } else {
                                                if ( cl->stats.glue <= 18.5f ) {
                                                    return 219.0/46.0;
                                                } else {
                                                    if ( cl->stats.num_antecedents_rel <= 0.796426713467f ) {
                                                        return 187.0/32.0;
                                                    } else {
                                                        return 361.0/18.0;
                                                    }
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 1.09192383289f ) {
                                            return 228.0/28.0;
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 1.16611278057f ) {
                                                return 193.0/70.0;
                                            } else {
                                                return 374.0/62.0;
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 232.5f ) {
                                if ( cl->stats.sum_uip1_used <= 11.5f ) {
                                    return 258.0/314.0;
                                } else {
                                    return 90.0/416.0;
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                                    return 139.0/184.0;
                                } else {
                                    return 232.0/144.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_long <= 0.928374111652f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 233074.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 1.20575618744f ) {
                                    if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                        return 230.0/86.0;
                                    } else {
                                        return 223.0/172.0;
                                    }
                                } else {
                                    return 196.0/48.0;
                                }
                            } else {
                                if ( cl->stats.dump_number <= 35.5f ) {
                                    return 216.0/26.0;
                                } else {
                                    return 232.0/68.0;
                                }
                            }
                        } else {
                            if ( cl->stats.num_antecedents_rel <= 0.791189908981f ) {
                                if ( rdb0_last_touched_diff <= 135015.0f ) {
                                    if ( cl->stats.sum_uip1_used <= 1.5f ) {
                                        if ( rdb0_last_touched_diff <= 116954.0f ) {
                                            return 204.0/20.0;
                                        } else {
                                            return 201.0/34.0;
                                        }
                                    } else {
                                        return 182.0/124.0;
                                    }
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 1.17793369293f ) {
                                        return 237.0/90.0;
                                    } else {
                                        if ( cl->size() <= 88.5f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 371745.0f ) {
                                                if ( cl->stats.dump_number <= 29.5f ) {
                                                    if ( cl->stats.glue_rel_long <= 1.11082458496f ) {
                                                        return 257.0/62.0;
                                                    } else {
                                                        if ( rdb0_last_touched_diff <= 175110.5f ) {
                                                            return 198.0/32.0;
                                                        } else {
                                                            return 326.0/12.0;
                                                        }
                                                    }
                                                } else {
                                                    return 183.0/68.0;
                                                }
                                            } else {
                                                return 288.0/24.0;
                                            }
                                        } else {
                                            return 394.0/18.0;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.size_rel <= 0.957697749138f ) {
                                    if ( cl->stats.sum_uip1_used <= 1.5f ) {
                                        if ( cl->stats.num_overlap_literals <= 646.0f ) {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 1.5f ) {
                                                return 204.0/44.0;
                                            } else {
                                                return 360.0/36.0;
                                            }
                                        } else {
                                            return 198.0/8.0;
                                        }
                                    } else {
                                        return 259.0/90.0;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 1.49233186245f ) {
                                        if ( cl->stats.size_rel <= 1.1229711771f ) {
                                            return 251.0/2.0;
                                        } else {
                                            if ( cl->stats.size_rel <= 1.46063637733f ) {
                                                if ( cl->stats.glue_rel_queue <= 1.22153449059f ) {
                                                    return 334.0/50.0;
                                                } else {
                                                    return 221.0/14.0;
                                                }
                                            } else {
                                                if ( cl->stats.glue_rel_queue <= 1.28532290459f ) {
                                                    if ( cl->stats.num_overlap_literals <= 203.5f ) {
                                                        return 231.0/14.0;
                                                    } else {
                                                        return 345.0/6.0;
                                                    }
                                                } else {
                                                    return 180.0/26.0;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.size_rel <= 1.41406035423f ) {
                                            return 214.0/2.0;
                                        } else {
                                            return 1;
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.antec_num_total_lits_rel <= 0.82086455822f ) {
                        return 99.0/384.0;
                    } else {
                        return 102.0/208.0;
                    }
                }
            }
        }
    } else {
        if ( rdb0_last_touched_diff <= 10006.0f ) {
            if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                if ( cl->stats.sum_uip1_used <= 13.5f ) {
                    if ( cl->stats.num_total_lits_antecedents <= 153.5f ) {
                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                            if ( cl->size() <= 10.5f ) {
                                if ( cl->stats.glue <= 5.5f ) {
                                    return 51.0/663.9;
                                } else {
                                    return 45.0/334.0;
                                }
                            } else {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                    return 139.0/514.0;
                                } else {
                                    return 47.0/392.0;
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.619006991386f ) {
                                return 62.0/380.0;
                            } else {
                                return 90.0/272.0;
                            }
                        }
                    } else {
                        return 130.0/312.0;
                    }
                } else {
                    if ( cl->stats.antec_num_total_lits_rel <= 0.26875269413f ) {
                        if ( cl->stats.glue_rel_long <= 0.267388194799f ) {
                            if ( cl->stats.size_rel <= 0.169406637549f ) {
                                if ( cl->stats.glue_rel_queue <= 0.200401604176f ) {
                                    return 54.0/400.0;
                                } else {
                                    return 39.0/398.0;
                                }
                            } else {
                                return 20.0/394.0;
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals_rel <= 0.0178902279586f ) {
                                if ( cl->size() <= 8.5f ) {
                                    return 32.0/456.0;
                                } else {
                                    return 55.0/488.0;
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 26.5f ) {
                                    if ( rdb0_last_touched_diff <= 5515.0f ) {
                                        if ( rdb0_last_touched_diff <= 2397.5f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0507910177112f ) {
                                                return 14.0/500.0;
                                            } else {
                                                return 9.0/667.9;
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 0.527356624603f ) {
                                                return 16.0/650.0;
                                            } else {
                                                return 28.0/414.0;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.444146811962f ) {
                                            return 27.0/646.0;
                                        } else {
                                            return 57.0/669.9;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.0976933836937f ) {
                                        return 43.0/364.0;
                                    } else {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 2202130.5f ) {
                                            return 53.0/498.0;
                                        } else {
                                            if ( cl->stats.dump_number <= 24.5f ) {
                                                return 11.0/480.0;
                                            } else {
                                                return 21.0/400.0;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.num_antecedents_rel <= 0.348527431488f ) {
                            return 82.0/568.0;
                        } else {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                return 48.0/378.0;
                            } else {
                                return 40.0/622.0;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.sum_delta_confl_uip1_used <= 567667.5f ) {
                    if ( cl->stats.glue <= 8.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 2552.5f ) {
                            if ( cl->stats.num_overlap_literals <= 4.5f ) {
                                return 4.0/392.0;
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.512853384018f ) {
                                    return 15.0/634.0;
                                } else {
                                    return 19.0/506.0;
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.482703447342f ) {
                                return 27.0/530.0;
                            } else {
                                return 40.0/334.0;
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_long <= 0.70868229866f ) {
                            return 33.0/406.0;
                        } else {
                            return 69.0/338.0;
                        }
                    }
                } else {
                    if ( cl->stats.sum_uip1_used <= 60.5f ) {
                        if ( cl->stats.num_overlap_literals <= 37.5f ) {
                            if ( cl->stats.size_rel <= 0.489874303341f ) {
                                if ( rdb0_last_touched_diff <= 924.0f ) {
                                    return 11.0/576.0;
                                } else {
                                    if ( rdb0_last_touched_diff <= 4661.5f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 17.5f ) {
                                            return 27.0/402.0;
                                        } else {
                                            return 13.0/486.0;
                                        }
                                    } else {
                                        return 28.0/370.0;
                                    }
                                }
                            } else {
                                return 39.0/422.0;
                            }
                        } else {
                            return 40.0/342.0;
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 104.5f ) {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 20130546.0f ) {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 6.5f ) {
                                    if ( cl->stats.used_for_uip_creation <= 12.5f ) {
                                        if ( rdb0_last_touched_diff <= 2334.5f ) {
                                            return 14.0/438.0;
                                        } else {
                                            return 19.0/382.0;
                                        }
                                    } else {
                                        return 6.0/640.0;
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.228712916374f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 17.5f ) {
                                            if ( cl->stats.num_overlap_literals <= 3.5f ) {
                                                return 2.0/657.9;
                                            } else {
                                                if ( cl->stats.glue <= 3.5f ) {
                                                    return 6.0/522.0;
                                                } else {
                                                    return 12.0/544.0;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.num_overlap_literals <= 12.5f ) {
                                                if ( cl->stats.rdb1_last_touched_diff <= 520.5f ) {
                                                    return 1.0/404.0;
                                                } else {
                                                    return 0.0/640.0;
                                                }
                                            } else {
                                                return 5.0/596.0;
                                            }
                                        }
                                    } else {
                                        return 17.0/687.9;
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 5484.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.199991106987f ) {
                                        return 22.0/508.0;
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.0405681878328f ) {
                                            return 27.0/705.9;
                                        } else {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.0666205585003f ) {
                                                if ( cl->stats.sum_delta_confl_uip1_used <= 64555328.0f ) {
                                                    return 2.0/408.0;
                                                } else {
                                                    return 4.0/368.0;
                                                }
                                            } else {
                                                if ( cl->stats.sum_uip1_used <= 534.0f ) {
                                                    if ( cl->size() <= 14.5f ) {
                                                        if ( cl->stats.size_rel <= 0.262042701244f ) {
                                                            return 11.0/388.0;
                                                        } else {
                                                            return 6.0/673.9;
                                                        }
                                                    } else {
                                                        return 15.0/392.0;
                                                    }
                                                } else {
                                                    if ( cl->stats.glue_rel_long <= 0.373360604048f ) {
                                                        return 5.0/412.0;
                                                    } else {
                                                        return 3.0/606.0;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    return 24.0/434.0;
                                }
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 1.37640213966f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 849.5f ) {
                                    return 7.0/518.0;
                                } else {
                                    return 22.0/518.0;
                                }
                            } else {
                                return 41.0/480.0;
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.num_total_lits_antecedents <= 159.5f ) {
                if ( cl->size() <= 10.5f ) {
                    if ( cl->stats.sum_delta_confl_uip1_used <= 98916.0f ) {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                            if ( cl->stats.num_overlap_literals <= 5.5f ) {
                                return 76.0/330.0;
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.74613404274f ) {
                                    return 166.0/462.0;
                                } else {
                                    return 104.0/180.0;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 3221.5f ) {
                                return 32.0/352.0;
                            } else {
                                return 78.0/412.0;
                            }
                        }
                    } else {
                        if ( cl->stats.dump_number <= 10.5f ) {
                            if ( cl->stats.sum_uip1_used <= 20.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 4745.0f ) {
                                    return 82.0/408.0;
                                } else {
                                    return 30.0/360.0;
                                }
                            } else {
                                if ( cl->size() <= 6.5f ) {
                                    return 28.0/747.9;
                                } else {
                                    return 33.0/458.0;
                                }
                            }
                        } else {
                            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0284339152277f ) {
                                    return 49.0/578.0;
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 28.5f ) {
                                        return 98.0/268.0;
                                    } else {
                                        if ( rdb0_last_touched_diff <= 13375.0f ) {
                                            return 40.0/556.0;
                                        } else {
                                            return 77.0/566.0;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.446082502604f ) {
                                        return 96.0/360.0;
                                    } else {
                                        return 63.0/298.0;
                                    }
                                } else {
                                    return 40.0/384.0;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.dump_number <= 1.5f ) {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 36.0f ) {
                            if ( cl->stats.glue_rel_long <= 0.715820848942f ) {
                                return 151.0/224.0;
                            } else {
                                if ( cl->stats.size_rel <= 1.02596950531f ) {
                                    return 302.0/180.0;
                                } else {
                                    return 187.0/62.0;
                                }
                            }
                        } else {
                            if ( cl->size() <= 23.5f ) {
                                return 156.0/512.0;
                            } else {
                                return 126.0/184.0;
                            }
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 52.5f ) {
                            if ( cl->stats.num_antecedents_rel <= 0.259265482426f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 195337.0f ) {
                                    return 142.0/428.0;
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.657462477684f ) {
                                        if ( cl->stats.sum_uip1_used <= 51.0f ) {
                                            return 135.0/476.0;
                                        } else {
                                            return 53.0/458.0;
                                        }
                                    } else {
                                        return 89.0/632.0;
                                    }
                                }
                            } else {
                                return 128.0/436.0;
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 18241.0f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 70.5f ) {
                                    return 139.0/436.0;
                                } else {
                                    if ( cl->stats.dump_number <= 11.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.739243388176f ) {
                                            return 42.0/386.0;
                                        } else {
                                            return 69.0/286.0;
                                        }
                                    } else {
                                        return 109.0/432.0;
                                    }
                                }
                            } else {
                                return 105.0/208.0;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.antecedents_glue_long_reds_var <= 11.4174146652f ) {
                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.69261777401f ) {
                        return 147.0/308.0;
                    } else {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 105.0f ) {
                            return 233.0/80.0;
                        } else {
                            return 162.0/400.0;
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 49.4049987793f ) {
                            return 219.0/88.0;
                        } else {
                            return 269.0/16.0;
                        }
                    } else {
                        return 191.0/210.0;
                    }
                }
            }
        }
    }
}

static double estimator_should_keep_long_conf4_cluster0_5(
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
    if ( cl->stats.sum_uip1_used <= 4.5f ) {
        if ( cl->stats.glue_rel_queue <= 0.890605092049f ) {
            if ( rdb0_last_touched_diff <= 70845.0f ) {
                if ( rdb0_last_touched_diff <= 13738.0f ) {
                    if ( cl->size() <= 10.5f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 31.5f ) {
                            if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                return 86.0/626.0;
                            } else {
                                return 88.0/364.0;
                            }
                        } else {
                            return 91.0/244.0;
                        }
                    } else {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 43.5f ) {
                                return 140.0/146.0;
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.190632671118f ) {
                                    return 116.0/316.0;
                                } else {
                                    return 172.0/248.0;
                                }
                            }
                        } else {
                            return 99.0/328.0;
                        }
                    }
                } else {
                    if ( cl->stats.num_overlap_literals <= 23.5f ) {
                        if ( cl->size() <= 10.5f ) {
                            if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                                if ( cl->stats.glue_rel_queue <= 0.648044884205f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0907225310802f ) {
                                        return 148.0/382.0;
                                    } else {
                                        return 93.0/392.0;
                                    }
                                } else {
                                    return 105.0/214.0;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0274015106261f ) {
                                    return 149.0/196.0;
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 10.5f ) {
                                        if ( cl->stats.size_rel <= 0.228509753942f ) {
                                            return 87.0/288.0;
                                        } else {
                                            return 154.0/306.0;
                                        }
                                    } else {
                                        return 144.0/244.0;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.523160815239f ) {
                                if ( cl->stats.dump_number <= 3.5f ) {
                                    return 121.0/262.0;
                                } else {
                                    return 173.0/254.0;
                                }
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 13.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.223893493414f ) {
                                        return 171.0/166.0;
                                    } else {
                                        return 207.0/116.0;
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 31.5f ) {
                                        return 131.0/320.0;
                                    } else {
                                        return 239.0/314.0;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 2.5f ) {
                                return 156.0/132.0;
                            } else {
                                if ( cl->stats.size_rel <= 0.51205265522f ) {
                                    return 103.0/234.0;
                                } else {
                                    return 160.0/202.0;
                                }
                            }
                        } else {
                            if ( cl->stats.glue <= 5.5f ) {
                                return 223.0/354.0;
                            } else {
                                if ( cl->size() <= 28.5f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 121.5f ) {
                                        if ( cl->stats.glue_rel_long <= 0.674962639809f ) {
                                            return 166.0/180.0;
                                        } else {
                                            if ( cl->stats.size_rel <= 0.540238857269f ) {
                                                return 187.0/64.0;
                                            } else {
                                                return 180.0/100.0;
                                            }
                                        }
                                    } else {
                                        return 208.0/292.0;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 20506.5f ) {
                                        return 228.0/76.0;
                                    } else {
                                        if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                            return 208.0/88.0;
                                        } else {
                                            return 158.0/150.0;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.num_overlap_literals <= 10.5f ) {
                    if ( rdb0_last_touched_diff <= 156981.0f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 16.5f ) {
                            return 215.0/332.0;
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.683353543282f ) {
                                return 326.0/180.0;
                            } else {
                                return 139.0/146.0;
                            }
                        }
                    } else {
                        if ( cl->stats.antec_num_total_lits_rel <= 0.175346463919f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 18.5f ) {
                                return 267.0/218.0;
                            } else {
                                return 261.0/128.0;
                            }
                        } else {
                            return 276.0/88.0;
                        }
                    }
                } else {
                    if ( cl->stats.sum_delta_confl_uip1_used <= 11.5f ) {
                        if ( cl->stats.glue <= 5.5f ) {
                            return 317.0/192.0;
                        } else {
                            if ( cl->stats.dump_number <= 11.5f ) {
                                if ( rdb0_last_touched_diff <= 105658.0f ) {
                                    if ( cl->stats.num_overlap_literals <= 40.5f ) {
                                        return 205.0/88.0;
                                    } else {
                                        return 235.0/52.0;
                                    }
                                } else {
                                    return 172.0/106.0;
                                }
                            } else {
                                if ( cl->stats.size_rel <= 0.807543158531f ) {
                                    if ( cl->stats.glue <= 7.5f ) {
                                        return 221.0/100.0;
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.727112650871f ) {
                                            return 228.0/86.0;
                                        } else {
                                            return 249.0/42.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 43.5f ) {
                                        if ( rdb0_last_touched_diff <= 260736.0f ) {
                                            return 210.0/22.0;
                                        } else {
                                            return 198.0/8.0;
                                        }
                                    } else {
                                        return 382.0/86.0;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->size() <= 8.5f ) {
                            return 182.0/214.0;
                        } else {
                            if ( cl->stats.size_rel <= 0.87210047245f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 244069.0f ) {
                                    if ( cl->stats.size_rel <= 0.316583365202f ) {
                                        return 152.0/136.0;
                                    } else {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 247.0f ) {
                                            return 260.0/128.0;
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 46.5f ) {
                                                return 143.0/140.0;
                                            } else {
                                                return 283.0/166.0;
                                            }
                                        }
                                    }
                                } else {
                                    return 318.0/112.0;
                                }
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 1839.0f ) {
                                    return 281.0/60.0;
                                } else {
                                    return 224.0/110.0;
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.sum_delta_confl_uip1_used <= 162.5f ) {
                if ( cl->stats.num_total_lits_antecedents <= 36.5f ) {
                    if ( cl->stats.dump_number <= 6.5f ) {
                        if ( cl->stats.antec_num_total_lits_rel <= 0.191668182611f ) {
                            return 134.0/192.0;
                        } else {
                            return 141.0/136.0;
                        }
                    } else {
                        if ( cl->stats.size_rel <= 0.510667085648f ) {
                            return 163.0/160.0;
                        } else {
                            return 260.0/68.0;
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_last_touched_diff <= 76572.0f ) {
                        if ( cl->stats.num_overlap_literals <= 88.5f ) {
                            if ( cl->size() <= 12.5f ) {
                                return 230.0/190.0;
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.14203107357f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.106933444738f ) {
                                        return 263.0/96.0;
                                    } else {
                                        return 220.0/136.0;
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 0.943120002747f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.384021580219f ) {
                                            return 247.0/46.0;
                                        } else {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.562907099724f ) {
                                                return 202.0/86.0;
                                            } else {
                                                return 175.0/140.0;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 28.2361106873f ) {
                                            if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                                                if ( cl->size() <= 26.5f ) {
                                                    return 176.0/60.0;
                                                } else {
                                                    return 191.0/72.0;
                                                }
                                            } else {
                                                if ( cl->size() <= 28.5f ) {
                                                    return 280.0/84.0;
                                                } else {
                                                    return 234.0/32.0;
                                                }
                                            }
                                        } else {
                                            return 234.0/14.0;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 0.5f ) {
                                if ( rdb0_last_touched_diff <= 37605.5f ) {
                                    return 159.0/82.0;
                                } else {
                                    return 251.0/42.0;
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 1.13990426064f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.704239428043f ) {
                                        return 383.0/48.0;
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 571.0f ) {
                                            if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                                                return 280.0/126.0;
                                            } else {
                                                if ( cl->stats.glue_rel_long <= 0.994944095612f ) {
                                                    return 195.0/80.0;
                                                } else {
                                                    return 334.0/62.0;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.size_rel <= 0.784097313881f ) {
                                                return 235.0/12.0;
                                            } else {
                                                return 289.0/42.0;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue <= 13.5f ) {
                                        return 244.0/106.0;
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.799548566341f ) {
                                            return 374.0/58.0;
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 800.5f ) {
                                                if ( cl->stats.sum_delta_confl_uip1_used <= 5.5f ) {
                                                    if ( cl->stats.antec_num_total_lits_rel <= 1.34250819683f ) {
                                                        return 1;
                                                    } else {
                                                        return 336.0/20.0;
                                                    }
                                                } else {
                                                    return 264.0/16.0;
                                                }
                                            } else {
                                                if ( cl->stats.rdb1_last_touched_diff <= 26249.5f ) {
                                                    return 204.0/10.0;
                                                } else {
                                                    return 194.0/28.0;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.size_rel <= 1.33920288086f ) {
                            if ( rdb0_last_touched_diff <= 667205.5f ) {
                                if ( cl->stats.glue_rel_queue <= 0.916389465332f ) {
                                    return 201.0/68.0;
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 0.5f ) {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 9.21848487854f ) {
                                            return 214.0/76.0;
                                        } else {
                                            return 272.0/34.0;
                                        }
                                    } else {
                                        if ( cl->stats.glue <= 8.5f ) {
                                            return 200.0/66.0;
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 1.6651301384f ) {
                                                if ( cl->stats.antecedents_glue_long_reds_var <= 187.604217529f ) {
                                                    if ( cl->stats.num_antecedents_rel <= 1.21386635303f ) {
                                                        if ( cl->stats.dump_number <= 38.5f ) {
                                                            if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                                                if ( cl->stats.rdb1_act_ranking_top_10 <= 6.5f ) {
                                                                    if ( cl->stats.antecedents_glue_long_reds_var <= 8.15586471558f ) {
                                                                        if ( cl->stats.antec_num_total_lits_rel <= 0.521855711937f ) {
                                                                            return 233.0/20.0;
                                                                        } else {
                                                                            return 1;
                                                                        }
                                                                    } else {
                                                                        if ( cl->stats.glue_rel_long <= 1.05447757244f ) {
                                                                            return 201.0/50.0;
                                                                        } else {
                                                                            if ( cl->stats.rdb1_act_ranking_top_10 <= 4.5f ) {
                                                                                return 282.0/10.0;
                                                                            } else {
                                                                                return 228.0/28.0;
                                                                            }
                                                                        }
                                                                    }
                                                                } else {
                                                                    if ( rdb0_act_ranking_top_10 <= 7.5f ) {
                                                                        return 202.0/40.0;
                                                                    } else {
                                                                        return 230.0/68.0;
                                                                    }
                                                                }
                                                            } else {
                                                                if ( cl->stats.dump_number <= 17.5f ) {
                                                                    return 287.0/6.0;
                                                                } else {
                                                                    return 234.0/16.0;
                                                                }
                                                            }
                                                        } else {
                                                            return 252.0/4.0;
                                                        }
                                                    } else {
                                                        if ( cl->stats.num_total_lits_antecedents <= 223.0f ) {
                                                            return 214.0/24.0;
                                                        } else {
                                                            return 194.0/48.0;
                                                        }
                                                    }
                                                } else {
                                                    return 243.0/2.0;
                                                }
                                            } else {
                                                if ( cl->stats.glue_rel_long <= 1.15814733505f ) {
                                                    return 393.0/2.0;
                                                } else {
                                                    return 400.0/28.0;
                                                }
                                            }
                                        }
                                    }
                                }
                            } else {
                                return 199.0/78.0;
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 1.32863879204f ) {
                                if ( cl->size() <= 55.5f ) {
                                    if ( cl->stats.num_overlap_literals <= 43.5f ) {
                                        return 245.0/6.0;
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 1.3155567646f ) {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 1.5f ) {
                                                return 182.0/32.0;
                                            } else {
                                                return 171.0/54.0;
                                            }
                                        } else {
                                            return 236.0/22.0;
                                        }
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 128223.0f ) {
                                        return 251.0/30.0;
                                    } else {
                                        if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                            return 1;
                                        } else {
                                            return 374.0/18.0;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.glue <= 17.5f ) {
                                    return 234.0/14.0;
                                } else {
                                    if ( cl->stats.glue <= 39.5f ) {
                                        return 1;
                                    } else {
                                        return 249.0/4.0;
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.rdb1_last_touched_diff <= 44594.5f ) {
                    if ( cl->stats.num_overlap_literals <= 26.5f ) {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.149444445968f ) {
                            return 98.0/302.0;
                        } else {
                            return 156.0/224.0;
                        }
                    } else {
                        if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                            if ( cl->stats.num_overlap_literals <= 169.5f ) {
                                return 229.0/322.0;
                            } else {
                                return 174.0/148.0;
                            }
                        } else {
                            if ( cl->size() <= 51.5f ) {
                                return 205.0/172.0;
                            } else {
                                return 194.0/82.0;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.size_rel <= 0.741332173347f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 173.5f ) {
                            if ( rdb0_last_touched_diff <= 150088.0f ) {
                                return 200.0/262.0;
                            } else {
                                return 166.0/106.0;
                            }
                        } else {
                            return 210.0/76.0;
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 153489.5f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 10.0755214691f ) {
                                return 317.0/174.0;
                            } else {
                                return 334.0/74.0;
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 128.0f ) {
                                return 200.0/48.0;
                            } else {
                                return 233.0/26.0;
                            }
                        }
                    }
                }
            }
        }
    } else {
        if ( cl->stats.rdb1_last_touched_diff <= 37311.5f ) {
            if ( rdb0_last_touched_diff <= 9957.5f ) {
                if ( cl->stats.rdb1_used_for_uip_creation <= 6.5f ) {
                    if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                        if ( cl->stats.glue_rel_queue <= 0.631673932076f ) {
                            if ( cl->stats.size_rel <= 0.533878922462f ) {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.149444445968f ) {
                                        if ( rdb0_last_touched_diff <= 6926.0f ) {
                                            if ( cl->size() <= 6.5f ) {
                                                if ( cl->stats.dump_number <= 19.5f ) {
                                                    return 25.0/354.0;
                                                } else {
                                                    return 42.0/338.0;
                                                }
                                            } else {
                                                return 41.0/626.0;
                                            }
                                        } else {
                                            return 66.0/330.0;
                                        }
                                    } else {
                                        return 105.0/470.0;
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 5209.5f ) {
                                        if ( cl->stats.glue <= 6.5f ) {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                                return 46.0/560.0;
                                            } else {
                                                if ( cl->stats.num_overlap_literals_rel <= 0.0392072349787f ) {
                                                    return 38.0/596.0;
                                                } else {
                                                    return 15.0/755.9;
                                                }
                                            }
                                        } else {
                                            return 40.0/412.0;
                                        }
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.0691337436438f ) {
                                            return 48.0/334.0;
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 1713.0f ) {
                                                return 43.0/370.0;
                                            } else {
                                                if ( cl->stats.sum_delta_confl_uip1_used <= 1766726.5f ) {
                                                    return 17.0/406.0;
                                                } else {
                                                    return 38.0/442.0;
                                                }
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 12.5f ) {
                                    return 39.0/386.0;
                                } else {
                                    return 85.0/356.0;
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 4596.0f ) {
                                if ( cl->size() <= 10.5f ) {
                                    return 43.0/648.0;
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.937840104103f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.216941922903f ) {
                                            return 69.0/436.0;
                                        } else {
                                            return 72.0/318.0;
                                        }
                                    } else {
                                        return 67.0/552.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_uip1_used <= 25.5f ) {
                                    if ( cl->size() <= 11.5f ) {
                                        return 48.0/404.0;
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 78.5f ) {
                                            return 82.0/290.0;
                                        } else {
                                            return 122.0/210.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 62.5f ) {
                                        return 47.0/342.0;
                                    } else {
                                        return 31.0/406.0;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.num_overlap_literals <= 94.5f ) {
                            if ( cl->stats.sum_uip1_used <= 16.5f ) {
                                if ( cl->stats.num_overlap_literals <= 15.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.471436798573f ) {
                                        return 37.0/606.0;
                                    } else {
                                        return 65.0/634.0;
                                    }
                                } else {
                                    return 90.0/630.0;
                                }
                            } else {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                    if ( cl->stats.dump_number <= 14.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.683693647385f ) {
                                            if ( rdb0_last_touched_diff <= 546.0f ) {
                                                return 8.0/480.0;
                                            } else {
                                                if ( cl->stats.dump_number <= 6.5f ) {
                                                    if ( cl->stats.sum_delta_confl_uip1_used <= 592337.0f ) {
                                                        return 23.0/354.0;
                                                    } else {
                                                        return 36.0/352.0;
                                                    }
                                                } else {
                                                    return 35.0/759.9;
                                                }
                                            }
                                        } else {
                                            return 15.0/628.0;
                                        }
                                    } else {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 18596512.0f ) {
                                            if ( cl->stats.size_rel <= 0.277976810932f ) {
                                                return 51.0/506.0;
                                            } else {
                                                return 43.0/626.0;
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 0.33197671175f ) {
                                                return 25.0/332.0;
                                            } else {
                                                if ( cl->stats.size_rel <= 0.386971056461f ) {
                                                    return 13.0/560.0;
                                                } else {
                                                    return 25.0/340.0;
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->size() <= 24.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.061342664063f ) {
                                            if ( cl->stats.size_rel <= 0.0940118730068f ) {
                                                return 13.0/436.0;
                                            } else {
                                                return 35.0/468.0;
                                            }
                                        } else {
                                            if ( cl->stats.size_rel <= 0.445017814636f ) {
                                                if ( cl->stats.rdb1_used_for_uip_creation <= 4.5f ) {
                                                    if ( cl->stats.glue_rel_queue <= 0.430438399315f ) {
                                                        return 7.0/396.0;
                                                    } else {
                                                        return 5.0/402.0;
                                                    }
                                                } else {
                                                    return 18.0/586.0;
                                                }
                                            } else {
                                                return 17.0/420.0;
                                            }
                                        }
                                    } else {
                                        return 46.0/663.9;
                                    }
                                }
                            }
                        } else {
                            return 85.0/608.0;
                        }
                    }
                } else {
                    if ( cl->stats.used_for_uip_creation <= 5.5f ) {
                        if ( cl->stats.glue <= 13.5f ) {
                            if ( rdb0_last_touched_diff <= 4021.0f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 1647.5f ) {
                                    if ( cl->stats.sum_uip1_used <= 113.5f ) {
                                        return 12.0/610.0;
                                    } else {
                                        return 18.0/396.0;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.462371826172f ) {
                                        return 37.0/422.0;
                                    } else {
                                        return 19.0/486.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 563052.0f ) {
                                    return 50.0/474.0;
                                } else {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 12.5f ) {
                                        return 49.0/606.0;
                                    } else {
                                        return 29.0/638.0;
                                    }
                                }
                            }
                        } else {
                            return 49.0/396.0;
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 52.5f ) {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 30.5f ) {
                                if ( cl->stats.sum_uip1_used <= 41.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.110510841012f ) {
                                        return 13.0/418.0;
                                    } else {
                                        return 23.0/388.0;
                                    }
                                } else {
                                    if ( cl->stats.glue <= 8.5f ) {
                                        if ( rdb0_last_touched_diff <= 4641.0f ) {
                                            if ( cl->stats.num_antecedents_rel <= 0.101165637374f ) {
                                                if ( rdb0_last_touched_diff <= 289.5f ) {
                                                    return 10.0/448.0;
                                                } else {
                                                    if ( cl->stats.rdb1_last_touched_diff <= 1462.5f ) {
                                                        return 2.0/813.9;
                                                    } else {
                                                        return 5.0/488.0;
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.rdb1_last_touched_diff <= 389.5f ) {
                                                    return 15.0/426.0;
                                                } else {
                                                    if ( cl->stats.used_for_uip_creation <= 17.5f ) {
                                                        return 13.0/622.0;
                                                    } else {
                                                        return 2.0/544.0;
                                                    }
                                                }
                                            }
                                        } else {
                                            return 18.0/390.0;
                                        }
                                    } else {
                                        return 18.0/446.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_uip1_used <= 143.5f ) {
                                    return 10.0/536.0;
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 128.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0451787188649f ) {
                                            if ( cl->stats.dump_number <= 21.5f ) {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.027124017477f ) {
                                                    return 3.0/410.0;
                                                } else {
                                                    return 1.0/440.0;
                                                }
                                            } else {
                                                return 6.0/438.0;
                                            }
                                        } else {
                                            return 1.0/642.0;
                                        }
                                    } else {
                                        return 8.0/414.0;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 45.5f ) {
                                return 42.0/356.0;
                            } else {
                                if ( cl->stats.sum_uip1_used <= 125.5f ) {
                                    if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                        return 27.0/418.0;
                                    } else {
                                        return 12.0/434.0;
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 12.5f ) {
                                        if ( rdb0_last_touched_diff <= 573.0f ) {
                                            return 2.0/430.0;
                                        } else {
                                            return 7.0/436.0;
                                        }
                                    } else {
                                        return 25.0/769.9;
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.glue_rel_long <= 0.713730573654f ) {
                    if ( cl->stats.size_rel <= 0.586049079895f ) {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                            if ( rdb0_last_touched_diff <= 35551.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 33.5f ) {
                                    if ( cl->stats.sum_uip1_used <= 33.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.049857892096f ) {
                                            if ( cl->stats.size_rel <= 0.111747808754f ) {
                                                return 76.0/414.0;
                                            } else {
                                                if ( cl->stats.num_total_lits_antecedents <= 11.5f ) {
                                                    return 97.0/410.0;
                                                } else {
                                                    if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                                                        return 101.0/296.0;
                                                    } else {
                                                        return 96.0/228.0;
                                                    }
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.sum_uip1_used <= 11.5f ) {
                                                if ( cl->stats.glue_rel_long <= 0.471020102501f ) {
                                                    return 66.0/286.0;
                                                } else {
                                                    return 55.0/282.0;
                                                }
                                            } else {
                                                if ( cl->stats.num_antecedents_rel <= 0.232322707772f ) {
                                                    if ( cl->stats.dump_number <= 11.5f ) {
                                                        return 32.0/392.0;
                                                    } else {
                                                        return 64.0/318.0;
                                                    }
                                                } else {
                                                    return 67.0/292.0;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 71240176.0f ) {
                                            if ( cl->stats.sum_uip1_used <= 167.0f ) {
                                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                                    if ( rdb0_last_touched_diff <= 15309.0f ) {
                                                        return 49.0/360.0;
                                                    } else {
                                                        if ( cl->stats.num_overlap_literals_rel <= 0.0445922464132f ) {
                                                            return 18.0/378.0;
                                                        } else {
                                                            return 42.0/418.0;
                                                        }
                                                    }
                                                } else {
                                                    if ( cl->stats.num_overlap_literals <= 6.5f ) {
                                                        if ( cl->stats.num_antecedents_rel <= 0.100063368678f ) {
                                                            return 62.0/296.0;
                                                        } else {
                                                            return 46.0/470.0;
                                                        }
                                                    } else {
                                                        return 73.0/406.0;
                                                    }
                                                }
                                            } else {
                                                return 31.0/612.0;
                                            }
                                        } else {
                                            return 22.0/558.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 9901808.0f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.141909644008f ) {
                                            if ( cl->size() <= 22.5f ) {
                                                return 83.0/244.0;
                                            } else {
                                                return 112.0/236.0;
                                            }
                                        } else {
                                            if ( cl->stats.dump_number <= 11.5f ) {
                                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.51530611515f ) {
                                                    return 58.0/396.0;
                                                } else {
                                                    return 83.0/396.0;
                                                }
                                            } else {
                                                return 148.0/494.0;
                                            }
                                        }
                                    } else {
                                        return 47.0/302.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_uip1_used <= 71.5f ) {
                                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                        return 72.0/304.0;
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 36.5f ) {
                                            if ( cl->stats.glue_rel_long <= 0.41873729229f ) {
                                                return 110.0/364.0;
                                            } else {
                                                return 182.0/326.0;
                                            }
                                        } else {
                                            return 71.0/290.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 36.5f ) {
                                        return 42.0/344.0;
                                    } else {
                                        return 23.0/330.0;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.124646052718f ) {
                                if ( cl->stats.glue <= 6.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.075634598732f ) {
                                        if ( cl->stats.dump_number <= 14.5f ) {
                                            return 48.0/687.9;
                                        } else {
                                            return 43.0/390.0;
                                        }
                                    } else {
                                        return 79.0/640.0;
                                    }
                                } else {
                                    return 105.0/354.0;
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 39.5f ) {
                                    if ( cl->stats.num_overlap_literals <= 7.5f ) {
                                        return 15.0/460.0;
                                    } else {
                                        return 44.0/536.0;
                                    }
                                } else {
                                    return 64.0/406.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                            if ( cl->stats.dump_number <= 26.5f ) {
                                if ( cl->stats.glue_rel_queue <= 0.578731894493f ) {
                                    return 165.0/464.0;
                                } else {
                                    return 118.0/482.0;
                                }
                            } else {
                                return 118.0/230.0;
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 44.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 4364.5f ) {
                                    return 69.0/334.0;
                                } else {
                                    return 131.0/332.0;
                                }
                            } else {
                                return 25.0/352.0;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_overlap_literals_rel <= 0.668488383293f ) {
                        if ( cl->stats.size_rel <= 0.68955385685f ) {
                            if ( cl->stats.sum_uip1_used <= 11.5f ) {
                                if ( cl->stats.glue_rel_long <= 0.824904203415f ) {
                                    return 138.0/384.0;
                                } else {
                                    if ( cl->stats.glue <= 9.5f ) {
                                        return 120.0/264.0;
                                    } else {
                                        return 144.0/232.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue <= 10.5f ) {
                                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0939556658268f ) {
                                            return 41.0/338.0;
                                        } else {
                                            return 47.0/316.0;
                                        }
                                    } else {
                                        if ( cl->stats.glue <= 6.5f ) {
                                            return 57.0/360.0;
                                        } else {
                                            return 122.0/386.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 11599.0f ) {
                                        return 36.0/398.0;
                                    } else {
                                        return 66.0/356.0;
                                    }
                                }
                            }
                        } else {
                            if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                                if ( cl->stats.glue_rel_queue <= 0.818438470364f ) {
                                    return 82.0/352.0;
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 13.5f ) {
                                        return 159.0/278.0;
                                    } else {
                                        return 135.0/472.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.965740561485f ) {
                                    return 141.0/226.0;
                                } else {
                                    return 129.0/340.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.sum_uip1_used <= 19.5f ) {
                            if ( cl->stats.size_rel <= 0.804154634476f ) {
                                return 143.0/144.0;
                            } else {
                                return 193.0/276.0;
                            }
                        } else {
                            return 61.0/300.0;
                        }
                    }
                }
            }
        } else {
            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                if ( cl->stats.num_total_lits_antecedents <= 43.5f ) {
                    if ( rdb0_last_touched_diff <= 4435.0f ) {
                        if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 21.5f ) {
                                return 66.0/342.0;
                            } else {
                                return 44.0/348.0;
                            }
                        } else {
                            return 32.0/590.0;
                        }
                    } else {
                        if ( cl->stats.dump_number <= 12.5f ) {
                            return 69.0/394.0;
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 18.5f ) {
                                return 85.0/420.0;
                            } else {
                                return 170.0/442.0;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 90.5f ) {
                            return 112.0/296.0;
                        } else {
                            return 162.0/218.0;
                        }
                    } else {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 1742693.5f ) {
                            return 140.0/342.0;
                        } else {
                            return 79.0/438.0;
                        }
                    }
                }
            } else {
                if ( cl->size() <= 10.5f ) {
                    if ( cl->stats.num_total_lits_antecedents <= 11.5f ) {
                        if ( cl->stats.rdb1_act_ranking_top_10 <= 6.5f ) {
                            if ( cl->stats.sum_uip1_used <= 27.5f ) {
                                return 130.0/258.0;
                            } else {
                                return 96.0/606.0;
                            }
                        } else {
                            return 153.0/294.0;
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 17.5f ) {
                            if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 5411791.5f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 622303.5f ) {
                                        return 142.0/252.0;
                                    } else {
                                        return 126.0/304.0;
                                    }
                                } else {
                                    return 94.0/406.0;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0601240098476f ) {
                                    return 207.0/372.0;
                                } else {
                                    return 96.0/232.0;
                                }
                            }
                        } else {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 10728902.0f ) {
                                if ( rdb0_last_touched_diff <= 83807.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.635564625263f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 54399.5f ) {
                                            return 133.0/340.0;
                                        } else {
                                            return 79.0/278.0;
                                        }
                                    } else {
                                        return 114.0/216.0;
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 28.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.600923418999f ) {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 185043.0f ) {
                                                return 102.0/212.0;
                                            } else {
                                                return 86.0/250.0;
                                            }
                                        } else {
                                            return 140.0/216.0;
                                        }
                                    } else {
                                        if ( cl->size() <= 7.5f ) {
                                            return 172.0/278.0;
                                        } else {
                                            if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                                return 153.0/120.0;
                                            } else {
                                                return 154.0/142.0;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.225886672735f ) {
                                    return 90.0/340.0;
                                } else {
                                    return 86.0/240.0;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.glue_rel_long <= 0.689467668533f ) {
                        if ( cl->stats.dump_number <= 28.5f ) {
                            if ( cl->stats.sum_uip1_used <= 8.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.148260295391f ) {
                                    return 132.0/236.0;
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.272313177586f ) {
                                        return 185.0/138.0;
                                    } else {
                                        return 130.0/174.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 114194.0f ) {
                                    if ( cl->stats.num_overlap_literals <= 36.5f ) {
                                        if ( rdb0_last_touched_diff <= 62319.5f ) {
                                            return 77.0/354.0;
                                        } else {
                                            if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                                return 127.0/262.0;
                                            } else {
                                                return 80.0/284.0;
                                            }
                                        }
                                    } else {
                                        return 133.0/216.0;
                                    }
                                } else {
                                    return 227.0/340.0;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 92093.5f ) {
                                return 177.0/384.0;
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 127532.0f ) {
                                    return 170.0/74.0;
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 44.5f ) {
                                        if ( cl->stats.size_rel <= 0.456909418106f ) {
                                            if ( cl->stats.sum_uip1_used <= 45.5f ) {
                                                return 193.0/134.0;
                                            } else {
                                                return 145.0/154.0;
                                            }
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.229099333286f ) {
                                                return 169.0/238.0;
                                            } else {
                                                return 154.0/154.0;
                                            }
                                        }
                                    } else {
                                        return 159.0/110.0;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 222125.5f ) {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 2061.5f ) {
                                return 260.0/100.0;
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 3779043.0f ) {
                                    if ( cl->stats.sum_uip1_used <= 35.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.99842107296f ) {
                                            if ( cl->stats.num_total_lits_antecedents <= 152.5f ) {
                                                if ( cl->stats.num_overlap_literals_rel <= 0.253951638937f ) {
                                                    if ( cl->stats.glue_rel_queue <= 0.996527314186f ) {
                                                        if ( rdb0_last_touched_diff <= 98482.0f ) {
                                                            return 232.0/256.0;
                                                        } else {
                                                            if ( rdb0_last_touched_diff <= 143299.0f ) {
                                                                return 177.0/84.0;
                                                            } else {
                                                                return 165.0/122.0;
                                                            }
                                                        }
                                                    } else {
                                                        return 159.0/248.0;
                                                    }
                                                } else {
                                                    if ( cl->stats.glue_rel_long <= 0.919018507004f ) {
                                                        return 151.0/286.0;
                                                    } else {
                                                        return 162.0/158.0;
                                                    }
                                                }
                                            } else {
                                                return 255.0/208.0;
                                            }
                                        } else {
                                            return 235.0/150.0;
                                        }
                                    } else {
                                        return 80.0/290.0;
                                    }
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 13883664.0f ) {
                                        return 173.0/314.0;
                                    } else {
                                        return 92.0/242.0;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.size_rel <= 1.01378679276f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.401438951492f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 549296.0f ) {
                                        return 175.0/114.0;
                                    } else {
                                        return 130.0/142.0;
                                    }
                                } else {
                                    return 175.0/84.0;
                                }
                            } else {
                                return 224.0/64.0;
                            }
                        }
                    }
                }
            }
        }
    }
}

static double estimator_should_keep_long_conf4_cluster0_6(
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
        if ( cl->stats.sum_delta_confl_uip1_used <= 705.5f ) {
            if ( cl->stats.num_total_lits_antecedents <= 36.5f ) {
                if ( cl->size() <= 10.5f ) {
                    if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                            return 95.0/562.0;
                        } else {
                            return 108.0/354.0;
                        }
                    } else {
                        return 201.0/408.0;
                    }
                } else {
                    if ( cl->stats.sum_uip1_used <= 1.5f ) {
                        if ( cl->stats.antec_num_total_lits_rel <= 0.180372178555f ) {
                            return 107.0/160.0;
                        } else {
                            return 130.0/128.0;
                        }
                    } else {
                        return 139.0/302.0;
                    }
                }
            } else {
                if ( cl->stats.sum_uip1_used <= 1.5f ) {
                    if ( cl->stats.glue <= 8.5f ) {
                        if ( cl->stats.glue_rel_long <= 0.624245226383f ) {
                            return 193.0/298.0;
                        } else {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                return 196.0/202.0;
                            } else {
                                return 178.0/102.0;
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_long <= 0.929618954659f ) {
                            if ( rdb0_last_touched_diff <= 23361.5f ) {
                                return 263.0/162.0;
                            } else {
                                if ( cl->size() <= 29.5f ) {
                                    return 282.0/152.0;
                                } else {
                                    return 230.0/38.0;
                                }
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals <= 83.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.648767411709f ) {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                        return 300.0/96.0;
                                    } else {
                                        return 259.0/32.0;
                                    }
                                } else {
                                    return 190.0/82.0;
                                }
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.480541110039f ) {
                                    return 343.0/52.0;
                                } else {
                                    if ( cl->stats.size_rel <= 0.579739332199f ) {
                                        return 252.0/4.0;
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 622.5f ) {
                                            if ( cl->stats.glue_rel_queue <= 1.05161249638f ) {
                                                return 178.0/38.0;
                                            } else {
                                                if ( cl->stats.sum_delta_confl_uip1_used <= 2.5f ) {
                                                    if ( cl->stats.num_antecedents_rel <= 1.18362474442f ) {
                                                        return 193.0/16.0;
                                                    } else {
                                                        return 186.0/14.0;
                                                    }
                                                } else {
                                                    return 212.0/30.0;
                                                }
                                            }
                                        } else {
                                            return 279.0/8.0;
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.antec_num_total_lits_rel <= 0.616542577744f ) {
                        if ( cl->stats.size_rel <= 0.755524456501f ) {
                            if ( cl->stats.sum_uip1_used <= 2.5f ) {
                                return 188.0/200.0;
                            } else {
                                return 123.0/242.0;
                            }
                        } else {
                            return 251.0/190.0;
                        }
                    } else {
                        if ( cl->stats.num_antecedents_rel <= 1.10578942299f ) {
                            return 265.0/180.0;
                        } else {
                            return 205.0/62.0;
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.sum_uip1_used <= 13.5f ) {
                if ( cl->stats.glue <= 9.5f ) {
                    if ( cl->stats.dump_number <= 5.5f ) {
                        if ( cl->stats.sum_uip1_used <= 3.5f ) {
                            if ( cl->stats.size_rel <= 0.23703533411f ) {
                                return 76.0/486.0;
                            } else {
                                if ( cl->stats.dump_number <= 2.5f ) {
                                    return 147.0/460.0;
                                } else {
                                    return 160.0/304.0;
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 11876.5f ) {
                                if ( cl->stats.dump_number <= 3.5f ) {
                                    if ( cl->size() <= 19.5f ) {
                                        if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 36936.5f ) {
                                                return 83.0/562.0;
                                            } else {
                                                if ( cl->stats.num_antecedents_rel <= 0.173135027289f ) {
                                                    return 33.0/346.0;
                                                } else {
                                                    return 21.0/376.0;
                                                }
                                            }
                                        } else {
                                            return 42.0/606.0;
                                        }
                                    } else {
                                        return 53.0/346.0;
                                    }
                                } else {
                                    return 82.0/570.0;
                                }
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.142401173711f ) {
                                    if ( cl->stats.sum_uip1_used <= 6.5f ) {
                                        return 60.0/336.0;
                                    } else {
                                        return 58.0/370.0;
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 0.560382604599f ) {
                                        if ( cl->stats.sum_uip1_used <= 7.5f ) {
                                            return 106.0/336.0;
                                        } else {
                                            return 63.0/342.0;
                                        }
                                    } else {
                                        return 88.0/244.0;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->size() <= 9.5f ) {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 133905.5f ) {
                                return 180.0/414.0;
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.509770989418f ) {
                                        return 86.0/540.0;
                                    } else {
                                        return 142.0/506.0;
                                    }
                                } else {
                                    if ( cl->size() <= 6.5f ) {
                                        return 99.0/362.0;
                                    } else {
                                        return 170.0/400.0;
                                    }
                                }
                            }
                        } else {
                            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.sum_uip1_used <= 4.5f ) {
                                    return 168.0/200.0;
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 20.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.158284217119f ) {
                                            return 72.0/318.0;
                                        } else {
                                            return 97.0/274.0;
                                        }
                                    } else {
                                        return 154.0/328.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.dump_number <= 27.5f ) {
                                    if ( cl->stats.sum_uip1_used <= 4.5f ) {
                                        return 238.0/236.0;
                                    } else {
                                        if ( cl->size() <= 15.5f ) {
                                            return 159.0/424.0;
                                        } else {
                                            return 179.0/214.0;
                                        }
                                    }
                                } else {
                                    return 183.0/142.0;
                                }
                            }
                        }
                    }
                } else {
                    if ( rdb0_last_touched_diff <= 42872.5f ) {
                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 1.43150877953f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.101970121264f ) {
                                    return 93.0/370.0;
                                } else {
                                    return 78.0/228.0;
                                }
                            } else {
                                if ( cl->stats.dump_number <= 11.5f ) {
                                    if ( cl->size() <= 53.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 5881.0f ) {
                                            return 84.0/300.0;
                                        } else {
                                            return 167.0/420.0;
                                        }
                                    } else {
                                        return 117.0/224.0;
                                    }
                                } else {
                                    return 113.0/198.0;
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.624770283699f ) {
                                return 70.0/278.0;
                            } else {
                                if ( cl->stats.glue <= 13.5f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                                        return 126.0/278.0;
                                    } else {
                                        return 128.0/202.0;
                                    }
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 3.77276229858f ) {
                                        return 111.0/208.0;
                                    } else {
                                        return 214.0/168.0;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.num_overlap_literals <= 70.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 55236.5f ) {
                                return 145.0/126.0;
                            } else {
                                return 182.0/206.0;
                            }
                        } else {
                            return 246.0/196.0;
                        }
                    }
                }
            } else {
                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                        if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                            if ( cl->stats.size_rel <= 0.49805688858f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.132081031799f ) {
                                    if ( rdb0_last_touched_diff <= 34083.5f ) {
                                        if ( cl->size() <= 7.5f ) {
                                            return 37.0/494.0;
                                        } else {
                                            return 60.0/460.0;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.376037538052f ) {
                                            return 85.0/250.0;
                                        } else {
                                            return 60.0/302.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue <= 6.5f ) {
                                        return 71.0/362.0;
                                    } else {
                                        return 95.0/302.0;
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 28818.5f ) {
                                    return 104.0/364.0;
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.675381422043f ) {
                                        return 112.0/244.0;
                                    } else {
                                        return 89.0/260.0;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.glue <= 7.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.0727420449257f ) {
                                    if ( cl->stats.sum_uip1_used <= 60.5f ) {
                                        return 65.0/350.0;
                                    } else {
                                        return 38.0/400.0;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 4.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0793192535639f ) {
                                            return 27.0/344.0;
                                        } else {
                                            return 11.0/450.0;
                                        }
                                    } else {
                                        if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                            if ( rdb0_last_touched_diff <= 3337.0f ) {
                                                return 30.0/428.0;
                                            } else {
                                                return 62.0/452.0;
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 16933.0f ) {
                                                return 26.0/506.0;
                                            } else {
                                                return 52.0/657.9;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 5.5f ) {
                                    if ( cl->stats.sum_uip1_used <= 49.5f ) {
                                        return 140.0/450.0;
                                    } else {
                                        return 42.0/464.0;
                                    }
                                } else {
                                    return 40.0/594.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.size_rel <= 0.689341366291f ) {
                            if ( rdb0_last_touched_diff <= 7829.5f ) {
                                if ( cl->stats.sum_uip1_used <= 46.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 30.5f ) {
                                        if ( cl->stats.glue_rel_long <= 0.602019667625f ) {
                                            if ( cl->stats.dump_number <= 12.5f ) {
                                                if ( cl->stats.sum_delta_confl_uip1_used <= 200894.5f ) {
                                                    return 27.0/402.0;
                                                } else {
                                                    if ( cl->stats.rdb1_last_touched_diff <= 4813.0f ) {
                                                        if ( cl->stats.glue_rel_long <= 0.35146856308f ) {
                                                            return 4.0/442.0;
                                                        } else {
                                                            return 17.0/620.0;
                                                        }
                                                    } else {
                                                        return 24.0/394.0;
                                                    }
                                                }
                                            } else {
                                                return 55.0/510.0;
                                            }
                                        } else {
                                            return 17.0/681.9;
                                        }
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 2.52277779579f ) {
                                            if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                                return 61.0/528.0;
                                            } else {
                                                return 42.0/681.9;
                                            }
                                        } else {
                                            return 47.0/348.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.203596889973f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 27.5f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0101901069283f ) {
                                                return 8.0/374.0;
                                            } else {
                                                if ( cl->stats.num_antecedents_rel <= 0.0938997864723f ) {
                                                    return 46.0/432.0;
                                                } else {
                                                    return 21.0/396.0;
                                                }
                                            }
                                        } else {
                                            return 11.0/418.0;
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.287237405777f ) {
                                            if ( cl->stats.num_antecedents_rel <= 0.0940821915865f ) {
                                                if ( cl->stats.num_overlap_literals_rel <= 0.0130093181506f ) {
                                                    if ( cl->stats.size_rel <= 0.138121575117f ) {
                                                        if ( cl->stats.used_for_uip_creation <= 8.5f ) {
                                                            return 20.0/432.0;
                                                        } else {
                                                            if ( cl->stats.size_rel <= 0.0655082762241f ) {
                                                                return 1.0/496.0;
                                                            } else {
                                                                return 6.0/406.0;
                                                            }
                                                        }
                                                    } else {
                                                        return 17.0/440.0;
                                                    }
                                                } else {
                                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0745470970869f ) {
                                                        if ( rdb0_last_touched_diff <= 1594.5f ) {
                                                            if ( cl->stats.rdb1_last_touched_diff <= 1613.5f ) {
                                                                return 9.0/723.9;
                                                            } else {
                                                                return 23.0/414.0;
                                                            }
                                                        } else {
                                                            if ( cl->stats.glue_rel_queue <= 0.373065769672f ) {
                                                                return 35.0/348.0;
                                                            } else {
                                                                return 32.0/590.0;
                                                            }
                                                        }
                                                    } else {
                                                        return 15.0/693.9;
                                                    }
                                                }
                                            } else {
                                                if ( rdb0_last_touched_diff <= 4461.5f ) {
                                                    if ( cl->stats.used_for_uip_creation <= 13.5f ) {
                                                        if ( cl->stats.sum_delta_confl_uip1_used <= 15438332.0f ) {
                                                            if ( cl->stats.num_overlap_literals_rel <= 0.0454620718956f ) {
                                                                return 10.0/422.0;
                                                            } else {
                                                                if ( cl->stats.antec_num_total_lits_rel <= 0.16033230722f ) {
                                                                    return 6.0/817.9;
                                                                } else {
                                                                    return 13.0/558.0;
                                                                }
                                                            }
                                                        } else {
                                                            if ( cl->stats.num_overlap_literals <= 10.5f ) {
                                                                if ( cl->stats.size_rel <= 0.28584831953f ) {
                                                                    return 29.0/626.0;
                                                                } else {
                                                                    return 10.0/518.0;
                                                                }
                                                            } else {
                                                                return 9.0/504.0;
                                                            }
                                                        }
                                                    } else {
                                                        if ( rdb0_last_touched_diff <= 2581.5f ) {
                                                            if ( cl->stats.num_overlap_literals <= 24.5f ) {
                                                                if ( cl->stats.num_total_lits_antecedents <= 16.5f ) {
                                                                    if ( cl->stats.num_antecedents_rel <= 0.141208082438f ) {
                                                                        return 1.0/434.0;
                                                                    } else {
                                                                        return 0.0/478.0;
                                                                    }
                                                                } else {
                                                                    if ( cl->stats.rdb1_last_touched_diff <= 489.0f ) {
                                                                        return 4.0/514.0;
                                                                    } else {
                                                                        return 2.0/671.9;
                                                                    }
                                                                }
                                                            } else {
                                                                return 13.0/390.0;
                                                            }
                                                        } else {
                                                            return 11.0/400.0;
                                                        }
                                                    }
                                                } else {
                                                    if ( cl->stats.num_overlap_literals <= 9.5f ) {
                                                        return 16.0/707.9;
                                                    } else {
                                                        return 26.0/418.0;
                                                    }
                                                }
                                            }
                                        } else {
                                            return 37.0/727.9;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->size() <= 27.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 64.5f ) {
                                        if ( cl->stats.size_rel <= 0.198345690966f ) {
                                            if ( cl->stats.glue_rel_queue <= 0.350569963455f ) {
                                                if ( cl->stats.glue_rel_long <= 0.20787191391f ) {
                                                    return 45.0/372.0;
                                                } else {
                                                    return 77.0/380.0;
                                                }
                                            } else {
                                                if ( cl->stats.dump_number <= 17.5f ) {
                                                    return 22.0/400.0;
                                                } else {
                                                    return 48.0/350.0;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.sum_uip1_used <= 91.5f ) {
                                                if ( cl->stats.sum_delta_confl_uip1_used <= 2244369.0f ) {
                                                    if ( cl->size() <= 8.5f ) {
                                                        return 15.0/402.0;
                                                    } else {
                                                        return 64.0/576.0;
                                                    }
                                                } else {
                                                    if ( cl->stats.antec_num_total_lits_rel <= 0.133837729692f ) {
                                                        return 64.0/316.0;
                                                    } else {
                                                        return 75.0/476.0;
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.dump_number <= 60.5f ) {
                                                    if ( rdb0_last_touched_diff <= 12030.5f ) {
                                                        return 13.0/386.0;
                                                    } else {
                                                        return 19.0/386.0;
                                                    }
                                                } else {
                                                    return 26.0/400.0;
                                                }
                                            }
                                        }
                                    } else {
                                        return 55.0/288.0;
                                    }
                                } else {
                                    return 62.0/284.0;
                                }
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 4.24499988556f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 4850744.5f ) {
                                    if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                                        return 66.0/274.0;
                                    } else {
                                        return 50.0/588.0;
                                    }
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                        return 41.0/508.0;
                                    } else {
                                        return 29.0/685.9;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.409871459007f ) {
                                    return 53.0/414.0;
                                } else {
                                    return 79.0/274.0;
                                }
                            }
                        }
                    }
                } else {
                    if ( rdb0_last_touched_diff <= 29560.5f ) {
                        if ( cl->stats.dump_number <= 38.5f ) {
                            if ( cl->stats.num_overlap_literals_rel <= 0.179391622543f ) {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 48.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0565639510751f ) {
                                            return 83.0/406.0;
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 0.347463667393f ) {
                                                return 24.0/450.0;
                                            } else {
                                                if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                                                    if ( cl->stats.size_rel <= 0.396555185318f ) {
                                                        return 50.0/306.0;
                                                    } else {
                                                        return 47.0/334.0;
                                                    }
                                                } else {
                                                    return 60.0/657.9;
                                                }
                                            }
                                        }
                                    } else {
                                        return 88.0/384.0;
                                    }
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 583771.0f ) {
                                        if ( rdb0_last_touched_diff <= 10103.5f ) {
                                            return 31.0/492.0;
                                        } else {
                                            return 72.0/420.0;
                                        }
                                    } else {
                                        if ( cl->stats.dump_number <= 12.5f ) {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 1254289.5f ) {
                                                return 24.0/426.0;
                                            } else {
                                                if ( cl->stats.sum_delta_confl_uip1_used <= 3089645.5f ) {
                                                    return 15.0/494.0;
                                                } else {
                                                    return 5.0/496.0;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 21.5f ) {
                                                return 37.0/444.0;
                                            } else {
                                                return 31.0/486.0;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 11339.5f ) {
                                    return 62.0/564.0;
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 27.5f ) {
                                        return 94.0/288.0;
                                    } else {
                                        return 65.0/430.0;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 23.5f ) {
                                return 72.0/472.0;
                            } else {
                                return 111.0/334.0;
                            }
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 18.5f ) {
                            if ( cl->stats.glue_rel_long <= 0.295680522919f ) {
                                return 100.0/276.0;
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 15590410.0f ) {
                                    return 108.0/512.0;
                                } else {
                                    return 32.0/346.0;
                                }
                            }
                        } else {
                            if ( cl->stats.num_antecedents_rel <= 0.163513362408f ) {
                                if ( cl->stats.glue <= 7.5f ) {
                                    return 137.0/382.0;
                                } else {
                                    return 181.0/370.0;
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 57332.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.679984092712f ) {
                                        if ( cl->stats.num_overlap_literals <= 15.5f ) {
                                            return 65.0/372.0;
                                        } else {
                                            return 87.0/320.0;
                                        }
                                    } else {
                                        return 131.0/380.0;
                                    }
                                } else {
                                    return 190.0/312.0;
                                }
                            }
                        }
                    }
                }
            }
        }
    } else {
        if ( cl->stats.antecedents_glue_long_reds_var <= 1.18890571594f ) {
            if ( cl->stats.rdb1_last_touched_diff <= 60855.5f ) {
                if ( rdb0_last_touched_diff <= 14258.5f ) {
                    if ( rdb0_last_touched_diff <= 5435.0f ) {
                        if ( cl->stats.used_for_uip_creation <= 5.5f ) {
                            if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                                return 99.0/492.0;
                            } else {
                                if ( cl->stats.glue <= 6.5f ) {
                                    return 50.0/492.0;
                                } else {
                                    return 56.0/352.0;
                                }
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 43.5f ) {
                                return 30.0/546.0;
                            } else {
                                return 7.0/506.0;
                            }
                        }
                    } else {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 777891.0f ) {
                            return 130.0/464.0;
                        } else {
                            return 50.0/330.0;
                        }
                    }
                } else {
                    if ( cl->stats.glue_rel_queue <= 0.773766815662f ) {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 15.5f ) {
                            if ( cl->stats.glue_rel_long <= 0.48646235466f ) {
                                return 156.0/286.0;
                            } else {
                                return 263.0/262.0;
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.567781567574f ) {
                                if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 30468.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0506314784288f ) {
                                            return 150.0/492.0;
                                        } else {
                                            return 89.0/528.0;
                                        }
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.213349103928f ) {
                                            if ( cl->stats.sum_uip1_used <= 29.5f ) {
                                                return 180.0/404.0;
                                            } else {
                                                return 50.0/322.0;
                                            }
                                        } else {
                                            return 114.0/244.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 154541.5f ) {
                                        return 116.0/474.0;
                                    } else {
                                        return 42.0/368.0;
                                    }
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.652605652809f ) {
                                        return 90.0/246.0;
                                    } else {
                                        return 100.0/348.0;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.643118917942f ) {
                                        return 90.0/222.0;
                                    } else {
                                        return 135.0/238.0;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.253653228283f ) {
                            if ( cl->size() <= 13.5f ) {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 4.5f ) {
                                    return 131.0/398.0;
                                } else {
                                    return 138.0/174.0;
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 119.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 32659.5f ) {
                                        return 175.0/310.0;
                                    } else {
                                        return 203.0/188.0;
                                    }
                                } else {
                                    return 229.0/150.0;
                                }
                            }
                        } else {
                            return 286.0/166.0;
                        }
                    }
                }
            } else {
                if ( cl->stats.size_rel <= 0.719901442528f ) {
                    if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 232665.0f ) {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 68.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 92.5f ) {
                                    if ( rdb0_last_touched_diff <= 97700.5f ) {
                                        return 240.0/204.0;
                                    } else {
                                        if ( cl->stats.glue <= 5.5f ) {
                                            return 252.0/202.0;
                                        } else {
                                            if ( cl->stats.dump_number <= 15.5f ) {
                                                return 271.0/108.0;
                                            } else {
                                                return 172.0/96.0;
                                            }
                                        }
                                    }
                                } else {
                                    return 182.0/70.0;
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 24.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.424442887306f ) {
                                        if ( cl->size() <= 4.5f ) {
                                            return 170.0/378.0;
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.159269273281f ) {
                                                if ( cl->stats.num_antecedents_rel <= 0.101471349597f ) {
                                                    return 168.0/278.0;
                                                } else {
                                                    return 140.0/166.0;
                                                }
                                            } else {
                                                return 149.0/286.0;
                                            }
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 91865.5f ) {
                                            return 119.0/360.0;
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 0.499535381794f ) {
                                                return 84.0/296.0;
                                            } else {
                                                if ( cl->stats.sum_delta_confl_uip1_used <= 121415.0f ) {
                                                    return 101.0/228.0;
                                                } else {
                                                    if ( cl->stats.rdb1_last_touched_diff <= 140962.5f ) {
                                                        return 151.0/216.0;
                                                    } else {
                                                        return 107.0/186.0;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                        if ( cl->stats.glue <= 6.5f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 107840.0f ) {
                                                return 157.0/278.0;
                                            } else {
                                                return 157.0/158.0;
                                            }
                                        } else {
                                            if ( cl->stats.sum_uip1_used <= 19.5f ) {
                                                if ( cl->stats.dump_number <= 17.5f ) {
                                                    return 159.0/146.0;
                                                } else {
                                                    return 177.0/78.0;
                                                }
                                            } else {
                                                return 126.0/220.0;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 140940.5f ) {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 227160.5f ) {
                                                return 181.0/302.0;
                                            } else {
                                                return 99.0/286.0;
                                            }
                                        } else {
                                            if ( cl->stats.dump_number <= 27.5f ) {
                                                return 136.0/206.0;
                                            } else {
                                                return 212.0/186.0;
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.27636474371f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 5422113.5f ) {
                                    if ( cl->size() <= 11.5f ) {
                                        if ( rdb0_act_ranking_top_10 <= 7.5f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.057246658951f ) {
                                                return 199.0/168.0;
                                            } else {
                                                return 169.0/242.0;
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 394147.5f ) {
                                                return 178.0/100.0;
                                            } else {
                                                return 156.0/114.0;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.dump_number <= 59.5f ) {
                                            if ( rdb0_last_touched_diff <= 310160.5f ) {
                                                return 167.0/70.0;
                                            } else {
                                                return 230.0/40.0;
                                            }
                                        } else {
                                            return 212.0/138.0;
                                        }
                                    }
                                } else {
                                    return 160.0/372.0;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 29.5f ) {
                                    return 191.0/54.0;
                                } else {
                                    return 193.0/92.0;
                                }
                            }
                        }
                    } else {
                        return 118.0/546.0;
                    }
                } else {
                    if ( cl->stats.sum_uip1_used <= 3.5f ) {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 2077.0f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.560276806355f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 110930.5f ) {
                                    return 278.0/114.0;
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.181559920311f ) {
                                        if ( cl->stats.size_rel <= 0.953606843948f ) {
                                            return 199.0/48.0;
                                        } else {
                                            return 374.0/34.0;
                                        }
                                    } else {
                                        return 304.0/80.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.548195004463f ) {
                                    return 192.0/46.0;
                                } else {
                                    return 197.0/18.0;
                                }
                            }
                        } else {
                            return 191.0/112.0;
                        }
                    } else {
                        if ( cl->stats.sum_uip1_used <= 28.5f ) {
                            if ( cl->stats.glue_rel_long <= 0.675304532051f ) {
                                return 162.0/186.0;
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0922726988792f ) {
                                    return 133.0/130.0;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 163575.0f ) {
                                        return 192.0/120.0;
                                    } else {
                                        return 179.0/66.0;
                                    }
                                }
                            }
                        } else {
                            return 125.0/258.0;
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.sum_delta_confl_uip1_used <= 988.5f ) {
                if ( cl->stats.num_total_lits_antecedents <= 70.5f ) {
                    if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                        if ( rdb0_last_touched_diff <= 186035.5f ) {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 17.5f ) {
                                if ( cl->stats.size_rel <= 0.958107113838f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.287599146366f ) {
                                        return 274.0/108.0;
                                    } else {
                                        return 167.0/148.0;
                                    }
                                } else {
                                    return 228.0/44.0;
                                }
                            } else {
                                return 205.0/192.0;
                            }
                        } else {
                            return 322.0/82.0;
                        }
                    } else {
                        if ( cl->size() <= 22.5f ) {
                            if ( cl->stats.glue_rel_long <= 0.782855808735f ) {
                                return 204.0/130.0;
                            } else {
                                if ( cl->stats.size_rel <= 0.822474718094f ) {
                                    return 196.0/76.0;
                                } else {
                                    return 198.0/38.0;
                                }
                            }
                        } else {
                            return 316.0/34.0;
                        }
                    }
                } else {
                    if ( cl->stats.glue_rel_queue <= 0.875616371632f ) {
                        if ( cl->stats.num_overlap_literals_rel <= 0.902791082859f ) {
                            if ( rdb0_last_touched_diff <= 50648.5f ) {
                                return 173.0/120.0;
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.240531474352f ) {
                                    return 221.0/76.0;
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 4.5f ) {
                                        return 379.0/42.0;
                                    } else {
                                        return 249.0/62.0;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.790079951286f ) {
                                return 280.0/172.0;
                            } else {
                                return 250.0/78.0;
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 93640.0f ) {
                            if ( cl->stats.glue_rel_queue <= 1.17376184464f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 65.0f ) {
                                    if ( rdb0_last_touched_diff <= 47498.0f ) {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 15.6484375f ) {
                                            return 281.0/68.0;
                                        } else {
                                            return 277.0/98.0;
                                        }
                                    } else {
                                        if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                            if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                                                return 247.0/14.0;
                                            } else {
                                                if ( cl->stats.glue_rel_long <= 1.03327465057f ) {
                                                    return 200.0/16.0;
                                                } else {
                                                    return 193.0/34.0;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 18.5859985352f ) {
                                                return 226.0/36.0;
                                            } else {
                                                return 269.0/64.0;
                                            }
                                        }
                                    }
                                } else {
                                    return 189.0/88.0;
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 70643.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 142.5f ) {
                                        return 255.0/66.0;
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.455400377512f ) {
                                            return 272.0/46.0;
                                        } else {
                                            if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                                if ( rdb0_last_touched_diff <= 40368.5f ) {
                                                    return 187.0/22.0;
                                                } else {
                                                    return 210.0/8.0;
                                                }
                                            } else {
                                                return 316.0/10.0;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 248.5f ) {
                                        return 266.0/96.0;
                                    } else {
                                        return 350.0/38.0;
                                    }
                                }
                            }
                        } else {
                            if ( cl->size() <= 95.5f ) {
                                if ( cl->stats.size_rel <= 1.62654340267f ) {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 8.5f ) {
                                        if ( cl->stats.sum_uip1_used <= 1.5f ) {
                                            if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                                if ( cl->stats.num_total_lits_antecedents <= 129.5f ) {
                                                    return 368.0/66.0;
                                                } else {
                                                    if ( cl->stats.glue_rel_queue <= 1.00817036629f ) {
                                                        return 245.0/42.0;
                                                    } else {
                                                        if ( cl->stats.sum_delta_confl_uip1_used <= 1.5f ) {
                                                            return 338.0/44.0;
                                                        } else {
                                                            if ( cl->stats.size_rel <= 0.89328289032f ) {
                                                                return 277.0/2.0;
                                                            } else {
                                                                return 194.0/4.0;
                                                            }
                                                        }
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.num_antecedents_rel <= 0.897163689137f ) {
                                                    return 313.0/34.0;
                                                } else {
                                                    if ( cl->stats.num_overlap_literals_rel <= 1.8024084568f ) {
                                                        return 179.0/50.0;
                                                    } else {
                                                        return 178.0/66.0;
                                                    }
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.9405169487f ) {
                                                return 229.0/30.0;
                                            } else {
                                                return 196.0/64.0;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.808788180351f ) {
                                            return 283.0/44.0;
                                        } else {
                                            if ( cl->stats.num_overlap_literals_rel <= 1.65603756905f ) {
                                                return 314.0/12.0;
                                            } else {
                                                return 231.0/20.0;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 162385.5f ) {
                                        return 230.0/24.0;
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 242041.0f ) {
                                            return 216.0/10.0;
                                        } else {
                                            return 340.0/6.0;
                                        }
                                    }
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                    return 1;
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                        return 322.0/30.0;
                                    } else {
                                        return 356.0/10.0;
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                    if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 86.5f ) {
                            return 137.0/344.0;
                        } else {
                            return 129.0/196.0;
                        }
                    } else {
                        return 86.0/586.0;
                    }
                } else {
                    if ( cl->stats.rdb1_last_touched_diff <= 141206.5f ) {
                        if ( cl->stats.glue_rel_long <= 0.90681630373f ) {
                            if ( cl->size() <= 10.5f ) {
                                return 162.0/334.0;
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                    if ( cl->size() <= 23.5f ) {
                                        if ( cl->stats.sum_uip1_used <= 8.5f ) {
                                            return 144.0/156.0;
                                        } else {
                                            return 97.0/278.0;
                                        }
                                    } else {
                                        return 235.0/274.0;
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 0.65655374527f ) {
                                        if ( cl->stats.dump_number <= 10.5f ) {
                                            return 107.0/210.0;
                                        } else {
                                            return 180.0/168.0;
                                        }
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 97.5f ) {
                                            return 198.0/132.0;
                                        } else {
                                            return 170.0/226.0;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.size_rel <= 1.46835267544f ) {
                                if ( cl->stats.num_overlap_literals <= 108.5f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 169802.5f ) {
                                        return 301.0/210.0;
                                    } else {
                                        return 149.0/308.0;
                                    }
                                } else {
                                    if ( cl->stats.glue <= 21.5f ) {
                                        return 193.0/78.0;
                                    } else {
                                        return 171.0/94.0;
                                    }
                                }
                            } else {
                                return 247.0/96.0;
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_long <= 0.759068131447f ) {
                            if ( rdb0_last_touched_diff <= 274202.0f ) {
                                if ( cl->stats.glue_rel_long <= 0.601463913918f ) {
                                    return 213.0/186.0;
                                } else {
                                    return 183.0/248.0;
                                }
                            } else {
                                return 321.0/198.0;
                            }
                        } else {
                            if ( cl->size() <= 15.5f ) {
                                return 183.0/120.0;
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.419583082199f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 7.92500019073f ) {
                                        return 195.0/130.0;
                                    } else {
                                        return 278.0/92.0;
                                    }
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 5.5f ) {
                                        return 304.0/46.0;
                                    } else {
                                        return 207.0/98.0;
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

static double estimator_should_keep_long_conf4_cluster0_7(
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
        if ( cl->stats.antec_num_total_lits_rel <= 0.618916749954f ) {
            if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                    if ( cl->stats.num_overlap_literals_rel <= 0.155645430088f ) {
                        if ( cl->stats.sum_uip1_used <= 5.5f ) {
                            if ( cl->stats.num_overlap_literals <= 9.5f ) {
                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                    if ( cl->size() <= 10.5f ) {
                                        return 96.0/454.0;
                                    } else {
                                        return 165.0/214.0;
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 21.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.469484716654f ) {
                                            return 84.0/254.0;
                                        } else {
                                            return 154.0/312.0;
                                        }
                                    } else {
                                        return 153.0/158.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.976446032524f ) {
                                    if ( cl->stats.glue <= 8.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0484096780419f ) {
                                            return 127.0/150.0;
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.162547469139f ) {
                                                return 114.0/322.0;
                                            } else {
                                                return 128.0/160.0;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.744059622288f ) {
                                            return 142.0/140.0;
                                        } else {
                                            return 146.0/116.0;
                                        }
                                    }
                                } else {
                                    return 213.0/88.0;
                                }
                            }
                        } else {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.184163630009f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 20262.0f ) {
                                    if ( rdb0_last_touched_diff <= 15373.0f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.512033998966f ) {
                                            return 113.0/406.0;
                                        } else {
                                            return 62.0/330.0;
                                        }
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.0374239161611f ) {
                                            return 78.0/282.0;
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 12.5f ) {
                                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                                    return 23.0/368.0;
                                                } else {
                                                    return 37.0/334.0;
                                                }
                                            } else {
                                                if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                                    if ( rdb0_last_touched_diff <= 20431.5f ) {
                                                        return 73.0/556.0;
                                                    } else {
                                                        if ( cl->stats.glue_rel_long <= 0.53649187088f ) {
                                                            if ( cl->stats.rdb1_last_touched_diff <= 14042.5f ) {
                                                                return 57.0/310.0;
                                                            } else {
                                                                return 53.0/422.0;
                                                            }
                                                        } else {
                                                            return 136.0/474.0;
                                                        }
                                                    }
                                                } else {
                                                    return 27.0/360.0;
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 95613.5f ) {
                                        if ( cl->size() <= 15.5f ) {
                                            if ( cl->stats.sum_uip1_used <= 46.5f ) {
                                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0932893753052f ) {
                                                        return 89.0/306.0;
                                                    } else {
                                                        return 61.0/312.0;
                                                    }
                                                } else {
                                                    if ( cl->stats.num_antecedents_rel <= 0.148897066712f ) {
                                                        return 195.0/386.0;
                                                    } else {
                                                        return 90.0/270.0;
                                                    }
                                                }
                                            } else {
                                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                                    return 75.0/452.0;
                                                } else {
                                                    if ( cl->stats.rdb1_last_touched_diff <= 38974.0f ) {
                                                        return 66.0/580.0;
                                                    } else {
                                                        return 57.0/306.0;
                                                    }
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.size_rel <= 0.408422350883f ) {
                                                return 95.0/266.0;
                                            } else {
                                                return 146.0/286.0;
                                            }
                                        }
                                    } else {
                                        return 146.0/300.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 57.5f ) {
                                    if ( cl->stats.sum_uip1_used <= 35.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.111108802259f ) {
                                            return 92.0/178.0;
                                        } else {
                                            return 122.0/326.0;
                                        }
                                    } else {
                                        return 59.0/446.0;
                                    }
                                } else {
                                    return 132.0/262.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue <= 8.5f ) {
                            if ( cl->stats.sum_uip1_used <= 40.5f ) {
                                if ( cl->stats.sum_uip1_used <= 6.5f ) {
                                    if ( cl->size() <= 13.5f ) {
                                        if ( cl->stats.num_overlap_literals <= 26.5f ) {
                                            return 149.0/372.0;
                                        } else {
                                            return 125.0/196.0;
                                        }
                                    } else {
                                        return 246.0/224.0;
                                    }
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                        return 97.0/372.0;
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.545653045177f ) {
                                            return 86.0/208.0;
                                        } else {
                                            return 110.0/190.0;
                                        }
                                    }
                                }
                            } else {
                                return 73.0/542.0;
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.816845178604f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 30401.5f ) {
                                    if ( cl->stats.size_rel <= 0.670822024345f ) {
                                        return 95.0/238.0;
                                    } else {
                                        return 77.0/254.0;
                                    }
                                } else {
                                    return 230.0/238.0;
                                }
                            } else {
                                if ( cl->size() <= 20.5f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 4.25396823883f ) {
                                        return 151.0/248.0;
                                    } else {
                                        return 147.0/124.0;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 44056.0f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 123.5f ) {
                                            return 198.0/186.0;
                                        } else {
                                            return 195.0/120.0;
                                        }
                                    } else {
                                        return 240.0/84.0;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_act_ranking_top_10 <= 4.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 22208.5f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.45679011941f ) {
                                if ( rdb0_last_touched_diff <= 7421.5f ) {
                                    if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                        if ( cl->stats.size_rel <= 0.534078836441f ) {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.0687039792538f ) {
                                                if ( cl->stats.rdb1_last_touched_diff <= 12889.0f ) {
                                                    return 75.0/584.0;
                                                } else {
                                                    return 56.0/320.0;
                                                }
                                            } else {
                                                if ( cl->stats.glue_rel_queue <= 0.334790050983f ) {
                                                    return 43.0/362.0;
                                                } else {
                                                    if ( cl->stats.glue_rel_queue <= 0.551673471928f ) {
                                                        if ( cl->stats.rdb1_last_touched_diff <= 11327.5f ) {
                                                            return 14.0/432.0;
                                                        } else {
                                                            return 23.0/400.0;
                                                        }
                                                    } else {
                                                        return 63.0/622.0;
                                                    }
                                                }
                                            }
                                        } else {
                                            return 90.0/440.0;
                                        }
                                    } else {
                                        if ( cl->stats.glue <= 5.5f ) {
                                            if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                                if ( rdb0_last_touched_diff <= 2031.0f ) {
                                                    return 33.0/606.0;
                                                } else {
                                                    return 44.0/494.0;
                                                }
                                            } else {
                                                return 26.0/582.0;
                                            }
                                        } else {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.128453999758f ) {
                                                return 33.0/548.0;
                                            } else {
                                                return 19.0/763.9;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 11824.5f ) {
                                        return 73.0/496.0;
                                    } else {
                                        return 63.0/288.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                                    if ( cl->stats.glue <= 7.5f ) {
                                        return 67.0/288.0;
                                    } else {
                                        return 104.0/290.0;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 22.5f ) {
                                        if ( cl->stats.dump_number <= 7.5f ) {
                                            return 51.0/392.0;
                                        } else {
                                            return 31.0/472.0;
                                        }
                                    } else {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 705672.0f ) {
                                            return 69.0/296.0;
                                        } else {
                                            return 36.0/324.0;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 23.5f ) {
                                if ( cl->stats.dump_number <= 20.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.852882385254f ) {
                                        if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                            if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                                                return 119.0/402.0;
                                            } else {
                                                return 73.0/340.0;
                                            }
                                        } else {
                                            return 67.0/462.0;
                                        }
                                    } else {
                                        return 95.0/230.0;
                                    }
                                } else {
                                    return 113.0/208.0;
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 1734.0f ) {
                                    if ( cl->stats.sum_uip1_used <= 86.5f ) {
                                        return 43.0/434.0;
                                    } else {
                                        return 16.0/390.0;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.457439213991f ) {
                                        if ( cl->stats.sum_uip1_used <= 74.5f ) {
                                            return 38.0/300.0;
                                        } else {
                                            return 38.0/394.0;
                                        }
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.156278610229f ) {
                                            return 69.0/274.0;
                                        } else {
                                            return 57.0/360.0;
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 119540.0f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.0413223132491f ) {
                                if ( rdb0_last_touched_diff <= 2412.5f ) {
                                    return 33.0/372.0;
                                } else {
                                    return 50.0/304.0;
                                }
                            } else {
                                return 81.0/292.0;
                            }
                        } else {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.17239138484f ) {
                                return 84.0/242.0;
                            } else {
                                return 101.0/212.0;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.dump_number <= 1.5f ) {
                    if ( cl->size() <= 9.5f ) {
                        if ( cl->stats.antec_num_total_lits_rel <= 0.155696451664f ) {
                            if ( cl->stats.num_antecedents_rel <= 0.0644321292639f ) {
                                return 22.0/476.0;
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 68771.0f ) {
                                    return 50.0/480.0;
                                } else {
                                    return 13.0/388.0;
                                }
                            }
                        } else {
                            return 45.0/416.0;
                        }
                    } else {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 6.5f ) {
                            if ( cl->stats.sum_uip1_used <= 4.5f ) {
                                return 225.0/344.0;
                            } else {
                                return 93.0/488.0;
                            }
                        } else {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.155681699514f ) {
                                return 45.0/466.0;
                            } else {
                                return 85.0/466.0;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.sum_uip1_used <= 18.5f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 33.5f ) {
                            if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                                if ( rdb0_last_touched_diff <= 12646.5f ) {
                                    return 49.0/338.0;
                                } else {
                                    return 132.0/544.0;
                                }
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 224492.5f ) {
                                    return 25.0/486.0;
                                } else {
                                    return 58.0/504.0;
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 11395.5f ) {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                                    return 108.0/444.0;
                                } else {
                                    return 49.0/360.0;
                                }
                            } else {
                                if ( cl->stats.dump_number <= 5.5f ) {
                                    return 82.0/306.0;
                                } else {
                                    return 119.0/232.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 47769640.0f ) {
                            if ( cl->stats.dump_number <= 39.5f ) {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 16.5f ) {
                                    if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                                        if ( cl->stats.sum_uip1_used <= 45.5f ) {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 1647704.0f ) {
                                                if ( cl->stats.num_antecedents_rel <= 0.147547572851f ) {
                                                    return 31.0/336.0;
                                                } else {
                                                    return 48.0/378.0;
                                                }
                                            } else {
                                                return 92.0/422.0;
                                            }
                                        } else {
                                            if ( rdb0_last_touched_diff <= 14995.5f ) {
                                                if ( cl->stats.num_antecedents_rel <= 0.119290851057f ) {
                                                    return 37.0/434.0;
                                                } else {
                                                    return 23.0/526.0;
                                                }
                                            } else {
                                                return 53.0/444.0;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.331971615553f ) {
                                            if ( cl->stats.glue_rel_queue <= 0.223377257586f ) {
                                                if ( cl->stats.glue <= 3.5f ) {
                                                    return 27.0/502.0;
                                                } else {
                                                    return 42.0/414.0;
                                                }
                                            } else {
                                                if ( cl->stats.num_overlap_literals_rel <= 0.0223981030285f ) {
                                                    if ( cl->stats.glue_rel_queue <= 0.574163198471f ) {
                                                        if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                                            return 59.0/560.0;
                                                        } else {
                                                            return 26.0/681.9;
                                                        }
                                                    } else {
                                                        return 19.0/526.0;
                                                    }
                                                } else {
                                                    if ( cl->stats.sum_uip1_used <= 64.5f ) {
                                                        if ( cl->stats.num_total_lits_antecedents <= 12.5f ) {
                                                            return 31.0/436.0;
                                                        } else {
                                                            if ( cl->stats.size_rel <= 0.491233408451f ) {
                                                                if ( cl->stats.glue_rel_queue <= 0.449539244175f ) {
                                                                    return 35.0/747.9;
                                                                } else {
                                                                    if ( cl->stats.num_overlap_literals <= 10.5f ) {
                                                                        return 9.0/550.0;
                                                                    } else {
                                                                        return 20.0/612.0;
                                                                    }
                                                                }
                                                            } else {
                                                                return 46.0/646.0;
                                                            }
                                                        }
                                                    } else {
                                                        if ( cl->stats.size_rel <= 0.462570697069f ) {
                                                            if ( cl->stats.sum_delta_confl_uip1_used <= 21889288.0f ) {
                                                                if ( cl->stats.num_total_lits_antecedents <= 41.5f ) {
                                                                    if ( cl->stats.rdb1_last_touched_diff <= 772.5f ) {
                                                                        return 5.0/398.0;
                                                                    } else {
                                                                        if ( cl->stats.glue_rel_queue <= 0.414594829082f ) {
                                                                            return 0.0/580.0;
                                                                        } else {
                                                                            return 3.0/677.9;
                                                                        }
                                                                    }
                                                                } else {
                                                                    return 13.0/384.0;
                                                                }
                                                            } else {
                                                                return 19.0/524.0;
                                                            }
                                                        } else {
                                                            return 24.0/544.0;
                                                        }
                                                    }
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.size_rel <= 0.649308681488f ) {
                                                return 44.0/350.0;
                                            } else {
                                                return 23.0/418.0;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0164021123201f ) {
                                        return 6.0/634.0;
                                    } else {
                                        if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0537713356316f ) {
                                                return 58.0/540.0;
                                            } else {
                                                return 24.0/578.0;
                                            }
                                        } else {
                                            if ( cl->size() <= 41.5f ) {
                                                if ( cl->stats.glue <= 6.5f ) {
                                                    if ( cl->stats.sum_delta_confl_uip1_used <= 15807616.0f ) {
                                                        if ( cl->stats.num_overlap_literals_rel <= 0.0515546947718f ) {
                                                            return 4.0/745.9;
                                                        } else {
                                                            return 12.0/580.0;
                                                        }
                                                    } else {
                                                        return 15.0/520.0;
                                                    }
                                                } else {
                                                    if ( cl->stats.sum_uip1_used <= 158.5f ) {
                                                        return 24.0/394.0;
                                                    } else {
                                                        return 9.0/458.0;
                                                    }
                                                }
                                            } else {
                                                return 3.0/562.0;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 11917.0f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 1721.0f ) {
                                        return 30.0/510.0;
                                    } else {
                                        return 55.0/388.0;
                                    }
                                } else {
                                    return 80.0/296.0;
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.177951455116f ) {
                                return 32.0/414.0;
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                    if ( cl->stats.glue <= 5.5f ) {
                                        return 25.0/681.9;
                                    } else {
                                        return 35.0/446.0;
                                    }
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 2177.5f ) {
                                        if ( cl->stats.size_rel <= 0.504101395607f ) {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 60926464.0f ) {
                                                return 0.0/434.0;
                                            } else {
                                                if ( cl->stats.used_for_uip_creation <= 5.5f ) {
                                                    return 15.0/412.0;
                                                } else {
                                                    if ( cl->stats.sum_uip1_used <= 474.0f ) {
                                                        return 11.0/470.0;
                                                    } else {
                                                        if ( cl->stats.glue_rel_queue <= 0.304460465908f ) {
                                                            return 6.0/470.0;
                                                        } else {
                                                            if ( cl->stats.sum_uip1_used <= 1014.0f ) {
                                                                return 2.0/494.0;
                                                            } else {
                                                                return 3.0/392.0;
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        } else {
                                            return 14.0/392.0;
                                        }
                                    } else {
                                        return 16.0/378.0;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.glue <= 11.5f ) {
                if ( cl->stats.sum_uip1_used <= 6.5f ) {
                    if ( cl->stats.dump_number <= 6.5f ) {
                        if ( cl->stats.antec_num_total_lits_rel <= 0.798463463783f ) {
                            return 135.0/222.0;
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.813670158386f ) {
                                return 144.0/264.0;
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 1.23536801338f ) {
                                    return 155.0/114.0;
                                } else {
                                    return 160.0/160.0;
                                }
                            }
                        }
                    } else {
                        return 309.0/196.0;
                    }
                } else {
                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                        if ( cl->size() <= 22.5f ) {
                            if ( cl->stats.glue_rel_long <= 0.573053359985f ) {
                                return 32.0/362.0;
                            } else {
                                return 85.0/586.0;
                            }
                        } else {
                            return 99.0/380.0;
                        }
                    } else {
                        return 131.0/360.0;
                    }
                }
            } else {
                if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                    if ( cl->stats.rdb1_last_touched_diff <= 9135.5f ) {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                            if ( cl->stats.glue <= 21.5f ) {
                                return 248.0/210.0;
                            } else {
                                return 193.0/56.0;
                            }
                        } else {
                            return 111.0/320.0;
                        }
                    } else {
                        if ( cl->stats.glue_rel_queue <= 1.00083470345f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 38051.0f ) {
                                return 232.0/170.0;
                            } else {
                                return 207.0/90.0;
                            }
                        } else {
                            if ( cl->stats.glue <= 22.5f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 13.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 1.12239050865f ) {
                                        return 185.0/28.0;
                                    } else {
                                        return 211.0/24.0;
                                    }
                                } else {
                                    return 157.0/116.0;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 1.78223180771f ) {
                                    return 245.0/54.0;
                                } else {
                                    return 223.0/14.0;
                                }
                            }
                        }
                    }
                } else {
                    if ( rdb0_last_touched_diff <= 3241.0f ) {
                        return 43.0/306.0;
                    } else {
                        return 85.0/248.0;
                    }
                }
            }
        }
    } else {
        if ( cl->stats.glue <= 8.5f ) {
            if ( cl->stats.sum_delta_confl_uip1_used <= 593.5f ) {
                if ( cl->size() <= 12.5f ) {
                    if ( cl->stats.rdb1_last_touched_diff <= 51479.5f ) {
                        if ( cl->stats.num_overlap_literals_rel <= 0.0361102074385f ) {
                            return 155.0/188.0;
                        } else {
                            if ( cl->stats.num_overlap_literals <= 10.5f ) {
                                return 135.0/462.0;
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.788437604904f ) {
                                    return 158.0/314.0;
                                } else {
                                    return 166.0/186.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.dump_number <= 35.5f ) {
                            if ( cl->stats.glue_rel_queue <= 0.354644536972f ) {
                                return 157.0/246.0;
                            } else {
                                if ( cl->stats.glue <= 6.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.111507743597f ) {
                                        return 185.0/208.0;
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.580833077431f ) {
                                            return 179.0/94.0;
                                        } else {
                                            return 231.0/248.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 123335.5f ) {
                                        return 208.0/164.0;
                                    } else {
                                        return 231.0/114.0;
                                    }
                                }
                            }
                        } else {
                            return 222.0/64.0;
                        }
                    }
                } else {
                    if ( rdb0_last_touched_diff <= 172129.5f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 48.5f ) {
                            if ( cl->stats.size_rel <= 0.664734244347f ) {
                                return 213.0/162.0;
                            } else {
                                return 184.0/176.0;
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 1.0489256382f ) {
                                return 295.0/196.0;
                            } else {
                                if ( cl->stats.size_rel <= 0.669265389442f ) {
                                    return 236.0/54.0;
                                } else {
                                    return 324.0/146.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 67.5f ) {
                            if ( cl->stats.glue_rel_queue <= 0.677474379539f ) {
                                return 188.0/64.0;
                            } else {
                                return 204.0/38.0;
                            }
                        } else {
                            return 358.0/46.0;
                        }
                    }
                }
            } else {
                if ( rdb0_last_touched_diff <= 70838.0f ) {
                    if ( rdb0_last_touched_diff <= 33124.0f ) {
                        if ( cl->stats.sum_uip1_used <= 6.5f ) {
                            if ( cl->stats.glue_rel_queue <= 0.569453120232f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.0908201858401f ) {
                                    return 69.0/288.0;
                                } else {
                                    return 94.0/270.0;
                                }
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.219653218985f ) {
                                    return 117.0/200.0;
                                } else {
                                    return 123.0/160.0;
                                }
                            }
                        } else {
                            if ( cl->size() <= 4.5f ) {
                                return 49.0/646.0;
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.095153182745f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.324550926685f ) {
                                        return 40.0/384.0;
                                    } else {
                                        return 89.0/588.0;
                                    }
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 1823028.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.664269208908f ) {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.193934977055f ) {
                                                return 82.0/328.0;
                                            } else {
                                                return 67.0/364.0;
                                            }
                                        } else {
                                            return 88.0/294.0;
                                        }
                                    } else {
                                        return 46.0/348.0;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_long <= 0.607926368713f ) {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 5205319.5f ) {
                                if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                    if ( rdb0_last_touched_diff <= 38938.0f ) {
                                        return 85.0/296.0;
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.350850403309f ) {
                                            return 131.0/380.0;
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 30.5f ) {
                                                return 205.0/370.0;
                                            } else {
                                                return 94.0/234.0;
                                            }
                                        }
                                    }
                                } else {
                                    return 120.0/516.0;
                                }
                            } else {
                                return 79.0/464.0;
                            }
                        } else {
                            if ( cl->size() <= 10.5f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 280572.0f ) {
                                    return 117.0/260.0;
                                } else {
                                    return 75.0/264.0;
                                }
                            } else {
                                return 263.0/346.0;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.sum_uip1_used <= 40.5f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 17.5f ) {
                            if ( cl->stats.size_rel <= 0.228070124984f ) {
                                if ( cl->stats.glue_rel_long <= 0.232268482447f ) {
                                    return 113.0/236.0;
                                } else {
                                    if ( cl->stats.dump_number <= 23.5f ) {
                                        return 122.0/194.0;
                                    } else {
                                        return 185.0/188.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 5.5f ) {
                                    return 110.0/270.0;
                                } else {
                                    return 155.0/198.0;
                                }
                            }
                        } else {
                            if ( cl->size() <= 12.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 140062.5f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 134548.5f ) {
                                        if ( rdb0_last_touched_diff <= 109315.5f ) {
                                            return 112.0/240.0;
                                        } else {
                                            return 109.0/188.0;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 87229.5f ) {
                                            return 134.0/164.0;
                                        } else {
                                            return 186.0/178.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 9.5f ) {
                                        return 227.0/146.0;
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.628283321857f ) {
                                            if ( cl->stats.size_rel <= 0.234770745039f ) {
                                                return 134.0/160.0;
                                            } else {
                                                return 247.0/194.0;
                                            }
                                        } else {
                                            return 175.0/216.0;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 37931.5f ) {
                                    if ( cl->stats.dump_number <= 15.5f ) {
                                        return 159.0/100.0;
                                    } else {
                                        return 207.0/78.0;
                                    }
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.330337166786f ) {
                                            return 203.0/146.0;
                                        } else {
                                            return 160.0/80.0;
                                        }
                                    } else {
                                        if ( cl->stats.sum_uip1_used <= 5.5f ) {
                                            return 168.0/138.0;
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 0.557999491692f ) {
                                                return 141.0/192.0;
                                            } else {
                                                return 145.0/154.0;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 177968.5f ) {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.0593938678503f ) {
                                return 101.0/282.0;
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 19.5f ) {
                                    return 71.0/456.0;
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.137083217502f ) {
                                        return 100.0/226.0;
                                    } else {
                                        return 79.0/348.0;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 78.5f ) {
                                return 119.0/180.0;
                            } else {
                                return 131.0/306.0;
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.rdb1_last_touched_diff <= 35640.0f ) {
                if ( cl->stats.dump_number <= 4.5f ) {
                    if ( cl->stats.glue_rel_long <= 0.908845901489f ) {
                        if ( cl->stats.sum_uip1_used <= 3.5f ) {
                            if ( cl->stats.glue_rel_long <= 0.747349500656f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 115.5f ) {
                                    return 135.0/224.0;
                                } else {
                                    return 151.0/112.0;
                                }
                            } else {
                                return 281.0/194.0;
                            }
                        } else {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                                return 117.0/350.0;
                            } else {
                                return 60.0/302.0;
                            }
                        }
                    } else {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 18.5f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.231111109257f ) {
                                return 171.0/90.0;
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 2.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 1.16861402988f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.434795439243f ) {
                                            return 271.0/104.0;
                                        } else {
                                            if ( cl->stats.size_rel <= 1.26458275318f ) {
                                                return 310.0/48.0;
                                            } else {
                                                return 182.0/66.0;
                                            }
                                        }
                                    } else {
                                        return 264.0/22.0;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.790788888931f ) {
                                        return 221.0/38.0;
                                    } else {
                                        return 372.0/38.0;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 24.6380195618f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 10605.0f ) {
                                    return 112.0/240.0;
                                } else {
                                    return 247.0/190.0;
                                }
                            } else {
                                return 257.0/72.0;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_total_lits_antecedents <= 204.0f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 16576.0f ) {
                            if ( rdb0_last_touched_diff <= 17440.5f ) {
                                return 57.0/298.0;
                            } else {
                                return 106.0/400.0;
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.769772291183f ) {
                                return 98.0/344.0;
                            } else {
                                return 195.0/382.0;
                            }
                        }
                    } else {
                        return 188.0/266.0;
                    }
                }
            } else {
                if ( cl->stats.sum_uip1_used <= 5.5f ) {
                    if ( cl->stats.antec_num_total_lits_rel <= 0.378058463335f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 64189.5f ) {
                            if ( cl->stats.glue_rel_long <= 1.03786194324f ) {
                                if ( cl->stats.sum_uip1_used <= 1.5f ) {
                                    return 177.0/90.0;
                                } else {
                                    return 140.0/172.0;
                                }
                            } else {
                                return 226.0/58.0;
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 520443.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 130.5f ) {
                                    if ( rdb0_last_touched_diff <= 263239.0f ) {
                                        if ( cl->stats.glue_rel_long <= 0.643956422806f ) {
                                            return 220.0/200.0;
                                        } else {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 635.0f ) {
                                                if ( cl->stats.num_antecedents_rel <= 0.209518015385f ) {
                                                    if ( rdb0_last_touched_diff <= 129337.5f ) {
                                                        return 221.0/62.0;
                                                    } else {
                                                        return 340.0/54.0;
                                                    }
                                                } else {
                                                    if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                                        return 223.0/80.0;
                                                    } else {
                                                        return 188.0/78.0;
                                                    }
                                                }
                                            } else {
                                                return 292.0/208.0;
                                            }
                                        }
                                    } else {
                                        if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                            return 208.0/18.0;
                                        } else {
                                            return 326.0/66.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.152663111687f ) {
                                        return 271.0/12.0;
                                    } else {
                                        return 244.0/28.0;
                                    }
                                }
                            } else {
                                return 210.0/150.0;
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_queue <= 0.924119532108f ) {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 38094.5f ) {
                                if ( cl->stats.glue_rel_queue <= 0.702137649059f ) {
                                    if ( cl->stats.glue_rel_long <= 0.64919501543f ) {
                                        return 187.0/68.0;
                                    } else {
                                        return 160.0/86.0;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.983954787254f ) {
                                        if ( cl->stats.dump_number <= 18.5f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 80707.5f ) {
                                                return 308.0/60.0;
                                            } else {
                                                if ( cl->stats.num_antecedents_rel <= 0.697117447853f ) {
                                                    return 209.0/64.0;
                                                } else {
                                                    return 170.0/128.0;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_act_ranking_top_10 <= 6.5f ) {
                                                return 246.0/24.0;
                                            } else {
                                                return 297.0/64.0;
                                            }
                                        }
                                    } else {
                                        return 237.0/28.0;
                                    }
                                }
                            } else {
                                return 133.0/116.0;
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 2.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.716143608093f ) {
                                    if ( cl->size() <= 17.5f ) {
                                        return 277.0/92.0;
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 146918.0f ) {
                                            if ( cl->stats.num_antecedents_rel <= 0.671587824821f ) {
                                                if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                                    return 323.0/36.0;
                                                } else {
                                                    if ( cl->stats.glue <= 15.5f ) {
                                                        return 180.0/86.0;
                                                    } else {
                                                        return 386.0/50.0;
                                                    }
                                                }
                                            } else {
                                                return 246.0/16.0;
                                            }
                                        } else {
                                            if ( cl->stats.num_overlap_literals <= 31.5f ) {
                                                return 273.0/34.0;
                                            } else {
                                                if ( cl->stats.num_overlap_literals_rel <= 0.525164186954f ) {
                                                    if ( cl->stats.num_total_lits_antecedents <= 141.5f ) {
                                                        return 281.0/8.0;
                                                    } else {
                                                        return 1;
                                                    }
                                                } else {
                                                    return 350.0/38.0;
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue <= 17.5f ) {
                                        if ( rdb0_last_touched_diff <= 99166.5f ) {
                                            if ( cl->stats.glue_rel_long <= 1.10378623009f ) {
                                                return 353.0/40.0;
                                            } else {
                                                if ( cl->stats.glue_rel_long <= 1.29043459892f ) {
                                                    return 200.0/86.0;
                                                } else {
                                                    return 190.0/36.0;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 1.09078681469f ) {
                                                if ( cl->stats.num_antecedents_rel <= 0.91284430027f ) {
                                                    return 199.0/26.0;
                                                } else {
                                                    if ( cl->stats.antecedents_glue_long_reds_var <= 11.6986713409f ) {
                                                        return 325.0/8.0;
                                                    } else {
                                                        return 186.0/26.0;
                                                    }
                                                }
                                            } else {
                                                if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                                    return 377.0/22.0;
                                                } else {
                                                    if ( rdb0_act_ranking_top_10 <= 7.5f ) {
                                                        return 217.0/86.0;
                                                    } else {
                                                        if ( cl->stats.num_overlap_literals_rel <= 1.42885434628f ) {
                                                            return 227.0/4.0;
                                                        } else {
                                                            return 199.0/40.0;
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 67973.0f ) {
                                            if ( rdb0_last_touched_diff <= 55603.5f ) {
                                                return 201.0/26.0;
                                            } else {
                                                return 181.0/32.0;
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_act_ranking_top_10 <= 8.5f ) {
                                                if ( cl->stats.rdb1_act_ranking_top_10 <= 6.5f ) {
                                                    if ( cl->stats.num_overlap_literals_rel <= 1.05272674561f ) {
                                                        return 238.0/20.0;
                                                    } else {
                                                        if ( cl->stats.size_rel <= 1.51916456223f ) {
                                                            if ( cl->stats.size_rel <= 0.875957131386f ) {
                                                                return 230.0/6.0;
                                                            } else {
                                                                return 1;
                                                            }
                                                        } else {
                                                            return 397.0/16.0;
                                                        }
                                                    }
                                                } else {
                                                    if ( cl->stats.dump_number <= 16.5f ) {
                                                        return 274.0/40.0;
                                                    } else {
                                                        return 271.0/18.0;
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.rdb1_act_ranking_top_10 <= 9.5f ) {
                                                    return 1;
                                                } else {
                                                    return 292.0/6.0;
                                                }
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 48.5f ) {
                                    return 169.0/98.0;
                                } else {
                                    if ( cl->size() <= 79.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 1.06827068329f ) {
                                            return 178.0/82.0;
                                        } else {
                                            return 308.0/68.0;
                                        }
                                    } else {
                                        return 256.0/24.0;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.dump_number <= 27.5f ) {
                        if ( cl->stats.antec_num_total_lits_rel <= 1.02873671055f ) {
                            if ( cl->stats.num_overlap_literals_rel <= 0.0922086462379f ) {
                                if ( rdb0_last_touched_diff <= 69944.0f ) {
                                    return 77.0/278.0;
                                } else {
                                    if ( cl->stats.size_rel <= 0.381255030632f ) {
                                        return 99.0/194.0;
                                    } else {
                                        return 215.0/282.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_uip1_used <= 17.5f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 5850.0f ) {
                                        return 191.0/86.0;
                                    } else {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 90892.0f ) {
                                            return 201.0/168.0;
                                        } else {
                                            return 196.0/270.0;
                                        }
                                    }
                                } else {
                                    return 188.0/432.0;
                                }
                            }
                        } else {
                            return 180.0/106.0;
                        }
                    } else {
                        if ( cl->stats.num_overlap_literals <= 73.5f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 5.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0648306161165f ) {
                                    return 154.0/150.0;
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.794176399708f ) {
                                        return 114.0/206.0;
                                    } else {
                                        return 137.0/166.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_uip1_used <= 21.5f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.108488015831f ) {
                                        return 182.0/86.0;
                                    } else {
                                        return 195.0/124.0;
                                    }
                                } else {
                                    return 238.0/278.0;
                                }
                            }
                        } else {
                            return 271.0/134.0;
                        }
                    }
                }
            }
        }
    }
}

static double estimator_should_keep_long_conf4_cluster0_8(
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
        if ( cl->stats.sum_delta_confl_uip1_used <= 803.5f ) {
            if ( cl->stats.num_total_lits_antecedents <= 72.5f ) {
                if ( cl->stats.size_rel <= 0.865786075592f ) {
                    if ( cl->stats.num_total_lits_antecedents <= 31.5f ) {
                        if ( cl->stats.sum_uip1_used <= 2.5f ) {
                            if ( cl->stats.size_rel <= 0.137023091316f ) {
                                return 85.0/276.0;
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0527533479035f ) {
                                    return 143.0/192.0;
                                } else {
                                    return 176.0/452.0;
                                }
                            }
                        } else {
                            return 100.0/460.0;
                        }
                    } else {
                        if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                            if ( cl->stats.glue_rel_long <= 0.822256803513f ) {
                                if ( cl->size() <= 14.5f ) {
                                    return 110.0/238.0;
                                } else {
                                    return 121.0/204.0;
                                }
                            } else {
                                return 145.0/146.0;
                            }
                        } else {
                            return 221.0/148.0;
                        }
                    }
                } else {
                    return 314.0/114.0;
                }
            } else {
                if ( cl->stats.glue_rel_long <= 0.875584483147f ) {
                    if ( cl->stats.glue_rel_long <= 0.639934420586f ) {
                        return 210.0/274.0;
                    } else {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 10.5f ) {
                            if ( cl->size() <= 25.5f ) {
                                return 175.0/76.0;
                            } else {
                                return 170.0/64.0;
                            }
                        } else {
                            return 162.0/120.0;
                        }
                    }
                } else {
                    if ( cl->size() <= 115.0f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 3390.5f ) {
                            return 194.0/106.0;
                        } else {
                            if ( cl->stats.glue_rel_queue <= 1.04275298119f ) {
                                if ( cl->stats.glue_rel_queue <= 0.990662932396f ) {
                                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                        return 181.0/40.0;
                                    } else {
                                        return 224.0/24.0;
                                    }
                                } else {
                                    return 157.0/60.0;
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 182.5f ) {
                                    if ( cl->stats.num_overlap_literals <= 65.5f ) {
                                        return 219.0/22.0;
                                    } else {
                                        return 265.0/72.0;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 384.5f ) {
                                        return 325.0/20.0;
                                    } else {
                                        return 219.0/30.0;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 72.0770874023f ) {
                            return 217.0/12.0;
                        } else {
                            return 214.0/2.0;
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                if ( cl->stats.num_total_lits_antecedents <= 21.5f ) {
                    if ( cl->stats.sum_delta_confl_uip1_used <= 19869898.0f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 37507.5f ) {
                            if ( cl->stats.dump_number <= 44.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 4126.5f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                        if ( cl->size() <= 7.5f ) {
                                            return 67.0/630.0;
                                        } else {
                                            return 60.0/336.0;
                                        }
                                    } else {
                                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                            if ( cl->stats.glue_rel_queue <= 0.374415874481f ) {
                                                return 36.0/368.0;
                                            } else {
                                                return 26.0/638.0;
                                            }
                                        } else {
                                            return 53.0/394.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 975696.5f ) {
                                        if ( cl->stats.dump_number <= 9.5f ) {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.09375f ) {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.086487300694f ) {
                                                    if ( cl->stats.glue_rel_queue <= 0.371517181396f ) {
                                                        return 72.0/288.0;
                                                    } else {
                                                        return 58.0/314.0;
                                                    }
                                                } else {
                                                    if ( cl->stats.num_overlap_literals_rel <= 0.0687179416418f ) {
                                                        return 38.0/350.0;
                                                    } else {
                                                        return 45.0/354.0;
                                                    }
                                                }
                                            } else {
                                                return 97.0/352.0;
                                            }
                                        } else {
                                            return 160.0/438.0;
                                        }
                                    } else {
                                        if ( cl->stats.glue <= 5.5f ) {
                                            if ( cl->stats.dump_number <= 20.5f ) {
                                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                                    return 42.0/614.0;
                                                } else {
                                                    return 44.0/366.0;
                                                }
                                            } else {
                                                if ( cl->stats.glue_rel_queue <= 0.335852473974f ) {
                                                    return 80.0/252.0;
                                                } else {
                                                    return 74.0/462.0;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.125882565975f ) {
                                                return 36.0/356.0;
                                            } else {
                                                return 30.0/396.0;
                                            }
                                        }
                                    }
                                }
                            } else {
                                return 156.0/376.0;
                            }
                        } else {
                            if ( cl->stats.size_rel <= 0.240019232035f ) {
                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                    return 80.0/256.0;
                                } else {
                                    return 118.0/224.0;
                                }
                            } else {
                                if ( cl->stats.dump_number <= 22.5f ) {
                                    return 64.0/338.0;
                                } else {
                                    return 82.0/256.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->size() <= 5.5f ) {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.0605991147459f ) {
                                return 48.0/474.0;
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 10642.5f ) {
                                    return 13.0/542.0;
                                } else {
                                    return 13.0/364.0;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 25071.0f ) {
                                if ( cl->stats.sum_uip1_used <= 274.5f ) {
                                    return 61.0/626.0;
                                } else {
                                    return 22.0/394.0;
                                }
                            } else {
                                return 45.0/298.0;
                            }
                        }
                    }
                } else {
                    if ( cl->size() <= 13.5f ) {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 4328388.0f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 54024.0f ) {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                        return 85.0/458.0;
                                    } else {
                                        return 103.0/258.0;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 73820.5f ) {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 1.86795902252f ) {
                                            if ( cl->stats.sum_uip1_used <= 9.5f ) {
                                                return 194.0/316.0;
                                            } else {
                                                if ( cl->stats.num_overlap_literals_rel <= 0.156149893999f ) {
                                                    if ( cl->stats.num_antecedents_rel <= 0.157613486052f ) {
                                                        return 100.0/236.0;
                                                    } else {
                                                        return 71.0/332.0;
                                                    }
                                                } else {
                                                    return 129.0/306.0;
                                                }
                                            }
                                        } else {
                                            return 100.0/418.0;
                                        }
                                    } else {
                                        return 125.0/206.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_uip1_used <= 56.5f ) {
                                    return 100.0/294.0;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 24192.5f ) {
                                        return 31.0/392.0;
                                    } else {
                                        return 79.0/404.0;
                                    }
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 11839.0f ) {
                                if ( cl->stats.glue_rel_queue <= 0.388980925083f ) {
                                    return 48.0/290.0;
                                } else {
                                    if ( rdb0_last_touched_diff <= 5694.5f ) {
                                        return 54.0/400.0;
                                    } else {
                                        if ( rdb0_last_touched_diff <= 10437.5f ) {
                                            return 28.0/544.0;
                                        } else {
                                            return 45.0/348.0;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_uip1_used <= 10.5f ) {
                                    if ( cl->stats.num_overlap_literals <= 20.5f ) {
                                        return 106.0/258.0;
                                    } else {
                                        return 75.0/276.0;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 23.5f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.209317266941f ) {
                                            return 86.0/500.0;
                                        } else {
                                            return 40.0/412.0;
                                        }
                                    } else {
                                        return 85.0/298.0;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 27451.0f ) {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.519848823547f ) {
                                if ( rdb0_last_touched_diff <= 14345.0f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 737147.5f ) {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.245000004768f ) {
                                                return 84.0/304.0;
                                            } else {
                                                return 185.0/374.0;
                                            }
                                        } else {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.208009213209f ) {
                                                return 31.0/384.0;
                                            } else {
                                                return 83.0/274.0;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.sum_uip1_used <= 150.0f ) {
                                            if ( cl->stats.num_total_lits_antecedents <= 47.5f ) {
                                                return 91.0/484.0;
                                            } else {
                                                if ( rdb0_last_touched_diff <= 10535.0f ) {
                                                    return 52.0/496.0;
                                                } else {
                                                    return 61.0/308.0;
                                                }
                                            }
                                        } else {
                                            return 21.0/430.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 3141969.5f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 57.5f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 8014.0f ) {
                                                return 112.0/208.0;
                                            } else {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.156574636698f ) {
                                                    return 107.0/352.0;
                                                } else {
                                                    return 113.0/506.0;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.dump_number <= 12.5f ) {
                                                if ( cl->stats.size_rel <= 0.634901285172f ) {
                                                    return 125.0/282.0;
                                                } else {
                                                    return 88.0/336.0;
                                                }
                                            } else {
                                                return 144.0/150.0;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.sum_uip1_used <= 96.0f ) {
                                            return 129.0/400.0;
                                        } else {
                                            return 49.0/526.0;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 361.0f ) {
                                        if ( cl->stats.sum_uip1_used <= 6.5f ) {
                                            return 192.0/232.0;
                                        } else {
                                            return 145.0/484.0;
                                        }
                                    } else {
                                        return 130.0/164.0;
                                    }
                                } else {
                                    return 121.0/424.0;
                                }
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 15.5f ) {
                                if ( cl->stats.glue <= 9.5f ) {
                                    if ( rdb0_last_touched_diff <= 62814.5f ) {
                                        if ( cl->stats.sum_uip1_used <= 5.5f ) {
                                            return 136.0/242.0;
                                        } else {
                                            return 94.0/230.0;
                                        }
                                    } else {
                                        return 157.0/194.0;
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 58242.0f ) {
                                        if ( cl->stats.size_rel <= 0.814486205578f ) {
                                            return 205.0/198.0;
                                        } else {
                                            return 162.0/226.0;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.841326951981f ) {
                                            return 146.0/138.0;
                                        } else {
                                            return 197.0/124.0;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.size_rel <= 0.397268712521f ) {
                                    return 83.0/338.0;
                                } else {
                                    if ( rdb0_last_touched_diff <= 74040.0f ) {
                                        if ( rdb0_last_touched_diff <= 43460.0f ) {
                                            return 82.0/296.0;
                                        } else {
                                            return 148.0/340.0;
                                        }
                                    } else {
                                        return 120.0/180.0;
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.antec_num_total_lits_rel <= 0.33586448431f ) {
                    if ( cl->stats.used_for_uip_creation <= 5.5f ) {
                        if ( cl->size() <= 9.5f ) {
                            if ( cl->stats.size_rel <= 0.268759608269f ) {
                                if ( rdb0_last_touched_diff <= 6624.0f ) {
                                    if ( cl->stats.sum_uip1_used <= 13.5f ) {
                                        return 90.0/628.0;
                                    } else {
                                        if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                            if ( rdb0_last_touched_diff <= 1142.5f ) {
                                                return 47.0/398.0;
                                            } else {
                                                if ( cl->stats.dump_number <= 19.5f ) {
                                                    return 32.0/713.9;
                                                } else {
                                                    return 49.0/530.0;
                                                }
                                            }
                                        } else {
                                            if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                                return 15.0/630.0;
                                            } else {
                                                return 33.0/659.9;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 46.5f ) {
                                        return 70.0/340.0;
                                    } else {
                                        return 27.0/350.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 22264.0f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.176778286695f ) {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 1777308.5f ) {
                                            return 34.0/512.0;
                                        } else {
                                            return 15.0/604.0;
                                        }
                                    } else {
                                        if ( cl->stats.sum_uip1_used <= 46.5f ) {
                                            return 19.0/538.0;
                                        } else {
                                            return 5.0/530.0;
                                        }
                                    }
                                } else {
                                    return 45.0/318.0;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.num_overlap_literals <= 10.5f ) {
                                    if ( cl->stats.dump_number <= 6.5f ) {
                                        return 54.0/488.0;
                                    } else {
                                        if ( cl->stats.sum_uip1_used <= 81.5f ) {
                                            return 54.0/560.0;
                                        } else {
                                            return 13.0/434.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.513849973679f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.382328867912f ) {
                                            return 42.0/346.0;
                                        } else {
                                            return 31.0/344.0;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.747996449471f ) {
                                            return 88.0/452.0;
                                        } else {
                                            return 54.0/460.0;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.654361367226f ) {
                                    if ( cl->stats.sum_uip1_used <= 33.5f ) {
                                        return 93.0/476.0;
                                    } else {
                                        return 28.0/362.0;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 18151.5f ) {
                                        return 57.0/298.0;
                                    } else {
                                        return 79.0/250.0;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.num_overlap_literals_rel <= 0.00459427712485f ) {
                            if ( cl->size() <= 5.5f ) {
                                return 9.0/492.0;
                            } else {
                                return 2.0/681.9;
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 2275.0f ) {
                                if ( cl->stats.used_for_uip_creation <= 11.5f ) {
                                    if ( cl->stats.sum_uip1_used <= 58.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0808004885912f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 5787.5f ) {
                                                return 22.0/602.0;
                                            } else {
                                                return 23.0/406.0;
                                            }
                                        } else {
                                            return 47.0/478.0;
                                        }
                                    } else {
                                        if ( cl->stats.size_rel <= 0.155626296997f ) {
                                            return 10.0/604.0;
                                        } else {
                                            if ( cl->stats.glue <= 5.5f ) {
                                                return 12.0/512.0;
                                            } else {
                                                return 27.0/606.0;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 4.5f ) {
                                        if ( cl->size() <= 11.5f ) {
                                            if ( cl->stats.num_antecedents_rel <= 0.0915382504463f ) {
                                                return 24.0/388.0;
                                            } else {
                                                if ( rdb0_last_touched_diff <= 620.0f ) {
                                                    return 3.0/418.0;
                                                } else {
                                                    return 10.0/410.0;
                                                }
                                            }
                                        } else {
                                            return 32.0/624.0;
                                        }
                                    } else {
                                        if ( cl->stats.glue <= 3.5f ) {
                                            if ( cl->stats.glue_rel_queue <= 0.358913004398f ) {
                                                if ( cl->size() <= 3.5f ) {
                                                    return 13.0/458.0;
                                                } else {
                                                    return 8.0/580.0;
                                                }
                                            } else {
                                                return 19.0/402.0;
                                            }
                                        } else {
                                            if ( cl->stats.size_rel <= 0.488581210375f ) {
                                                if ( cl->stats.num_antecedents_rel <= 0.0600796118379f ) {
                                                    return 18.0/755.9;
                                                } else {
                                                    if ( cl->stats.num_overlap_literals_rel <= 0.105007097125f ) {
                                                        if ( cl->stats.glue_rel_long <= 0.279723644257f ) {
                                                            return 8.0/366.0;
                                                        } else {
                                                            if ( cl->stats.num_overlap_literals <= 7.5f ) {
                                                                if ( cl->stats.dump_number <= 9.5f ) {
                                                                    return 1.0/418.0;
                                                                } else {
                                                                    return 0.0/534.0;
                                                                }
                                                            } else {
                                                                return 4.0/580.0;
                                                            }
                                                        }
                                                    } else {
                                                        return 12.0/506.0;
                                                    }
                                                }
                                            } else {
                                                return 18.0/570.0;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->size() <= 11.5f ) {
                                    if ( cl->stats.used_for_uip_creation <= 11.5f ) {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 10156848.0f ) {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                                return 50.0/398.0;
                                            } else {
                                                return 34.0/671.9;
                                            }
                                        } else {
                                            return 28.0/654.0;
                                        }
                                    } else {
                                        if ( cl->stats.sum_uip1_used <= 67.5f ) {
                                            return 28.0/506.0;
                                        } else {
                                            if ( cl->stats.dump_number <= 13.5f ) {
                                                return 17.0/554.0;
                                            } else {
                                                return 6.0/729.9;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 8.5f ) {
                                        if ( cl->stats.sum_uip1_used <= 49.5f ) {
                                            return 71.0/534.0;
                                        } else {
                                            return 31.0/468.0;
                                        }
                                    } else {
                                        if ( cl->stats.dump_number <= 7.5f ) {
                                            return 23.0/378.0;
                                        } else {
                                            return 14.0/410.0;
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.653333306313f ) {
                        if ( cl->stats.sum_uip1_used <= 14.5f ) {
                            return 69.0/318.0;
                        } else {
                            if ( cl->stats.num_overlap_literals_rel <= 0.286851406097f ) {
                                return 14.0/582.0;
                            } else {
                                if ( cl->stats.sum_uip1_used <= 109.0f ) {
                                    return 51.0/486.0;
                                } else {
                                    return 13.0/368.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->size() <= 9.5f ) {
                            return 43.0/398.0;
                        } else {
                            if ( cl->stats.dump_number <= 2.5f ) {
                                return 121.0/390.0;
                            } else {
                                if ( rdb0_last_touched_diff <= 5464.5f ) {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                        if ( cl->stats.used_for_uip_creation <= 6.5f ) {
                                            return 36.0/392.0;
                                        } else {
                                            return 23.0/458.0;
                                        }
                                    } else {
                                        return 92.0/420.0;
                                    }
                                } else {
                                    return 111.0/410.0;
                                }
                            }
                        }
                    }
                }
            }
        }
    } else {
        if ( cl->stats.antecedents_glue_long_reds_var <= 2.33301830292f ) {
            if ( rdb0_last_touched_diff <= 73203.0f ) {
                if ( cl->stats.glue <= 7.5f ) {
                    if ( cl->stats.sum_uip1_used <= 3.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 15793.5f ) {
                            if ( cl->stats.size_rel <= 0.383607268333f ) {
                                return 153.0/494.0;
                            } else {
                                return 114.0/184.0;
                            }
                        } else {
                            if ( cl->size() <= 13.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.0806315913796f ) {
                                    return 150.0/148.0;
                                } else {
                                    if ( cl->size() <= 5.5f ) {
                                        return 88.0/246.0;
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 10.5f ) {
                                            return 134.0/242.0;
                                        } else {
                                            return 213.0/244.0;
                                        }
                                    }
                                }
                            } else {
                                return 239.0/142.0;
                            }
                        }
                    } else {
                        if ( cl->stats.dump_number <= 23.5f ) {
                            if ( rdb0_last_touched_diff <= 43490.5f ) {
                                if ( cl->stats.sum_uip1_used <= 43.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 58.5f ) {
                                        if ( cl->stats.dump_number <= 5.5f ) {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                                if ( cl->stats.glue_rel_queue <= 0.456210464239f ) {
                                                    return 57.0/402.0;
                                                } else {
                                                    return 97.0/418.0;
                                                }
                                            } else {
                                                return 66.0/544.0;
                                            }
                                        } else {
                                            if ( cl->stats.sum_uip1_used <= 15.5f ) {
                                                return 127.0/444.0;
                                            } else {
                                                return 83.0/492.0;
                                            }
                                        }
                                    } else {
                                        return 114.0/378.0;
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.0947150588036f ) {
                                        return 31.0/382.0;
                                    } else {
                                        return 23.0/516.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 615850.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 30.5f ) {
                                        if ( cl->stats.sum_uip1_used <= 10.5f ) {
                                            return 93.0/276.0;
                                        } else {
                                            return 98.0/224.0;
                                        }
                                    } else {
                                        return 125.0/170.0;
                                    }
                                } else {
                                    return 89.0/440.0;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 41276.0f ) {
                                return 143.0/462.0;
                            } else {
                                return 133.0/330.0;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 856.5f ) {
                            if ( cl->stats.glue <= 9.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.268677592278f ) {
                                    return 185.0/204.0;
                                } else {
                                    return 203.0/144.0;
                                }
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.862286925316f ) {
                                    if ( cl->stats.size_rel <= 0.85024702549f ) {
                                        if ( cl->stats.num_overlap_literals <= 15.5f ) {
                                            return 146.0/152.0;
                                        } else {
                                            return 208.0/134.0;
                                        }
                                    } else {
                                        return 318.0/110.0;
                                    }
                                } else {
                                    return 183.0/36.0;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 20705.5f ) {
                                if ( cl->stats.num_overlap_literals <= 9.5f ) {
                                    return 121.0/304.0;
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 7.5f ) {
                                        return 92.0/232.0;
                                    } else {
                                        return 74.0/416.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 5.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.145727217197f ) {
                                        return 228.0/280.0;
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.838090240955f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.137089163065f ) {
                                                return 75.0/248.0;
                                            } else {
                                                return 109.0/244.0;
                                            }
                                        } else {
                                            return 184.0/232.0;
                                        }
                                    }
                                } else {
                                    return 95.0/368.0;
                                }
                            }
                        }
                    } else {
                        return 104.0/530.0;
                    }
                }
            } else {
                if ( cl->stats.sum_delta_confl_uip1_used <= 2708.0f ) {
                    if ( cl->stats.glue <= 8.5f ) {
                        if ( cl->stats.size_rel <= 0.305060207844f ) {
                            if ( cl->stats.dump_number <= 25.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.0696859061718f ) {
                                    return 167.0/106.0;
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0730282813311f ) {
                                        return 149.0/230.0;
                                    } else {
                                        return 193.0/246.0;
                                    }
                                }
                            } else {
                                return 242.0/144.0;
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.945786237717f ) {
                                if ( cl->stats.size_rel <= 0.876060068607f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 108167.5f ) {
                                        return 212.0/158.0;
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.721938788891f ) {
                                            if ( cl->stats.size_rel <= 0.537031114101f ) {
                                                return 262.0/142.0;
                                            } else {
                                                return 245.0/74.0;
                                            }
                                        } else {
                                            return 218.0/52.0;
                                        }
                                    }
                                } else {
                                    return 239.0/50.0;
                                }
                            } else {
                                return 159.0/130.0;
                            }
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.0131482835859f ) {
                            if ( cl->stats.glue_rel_queue <= 0.685485005379f ) {
                                return 211.0/170.0;
                            } else {
                                if ( cl->stats.dump_number <= 23.5f ) {
                                    if ( cl->stats.num_overlap_literals <= 20.5f ) {
                                        return 311.0/170.0;
                                    } else {
                                        return 230.0/30.0;
                                    }
                                } else {
                                    return 308.0/48.0;
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.851338386536f ) {
                                if ( cl->stats.num_overlap_literals <= 36.5f ) {
                                    return 308.0/50.0;
                                } else {
                                    return 189.0/74.0;
                                }
                            } else {
                                if ( cl->size() <= 16.5f ) {
                                    return 206.0/42.0;
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.432662159204f ) {
                                        return 340.0/42.0;
                                    } else {
                                        if ( rdb0_last_touched_diff <= 161874.0f ) {
                                            return 228.0/10.0;
                                        } else {
                                            return 322.0/2.0;
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.sum_delta_confl_uip1_used <= 5419926.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 246318.5f ) {
                            if ( cl->stats.dump_number <= 35.5f ) {
                                if ( cl->stats.sum_uip1_used <= 5.5f ) {
                                    if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.263780474663f ) {
                                            return 263.0/238.0;
                                        } else {
                                            return 207.0/124.0;
                                        }
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.263311386108f ) {
                                            return 192.0/252.0;
                                        } else {
                                            return 169.0/140.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 46.5f ) {
                                        if ( cl->stats.size_rel <= 0.773925065994f ) {
                                            if ( cl->stats.sum_uip1_used <= 22.5f ) {
                                                if ( cl->stats.num_overlap_literals_rel <= 0.0239123050123f ) {
                                                    return 135.0/144.0;
                                                } else {
                                                    if ( rdb0_last_touched_diff <= 93304.5f ) {
                                                        return 132.0/198.0;
                                                    } else {
                                                        if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                                            return 167.0/312.0;
                                                        } else {
                                                            if ( cl->stats.num_total_lits_antecedents <= 27.5f ) {
                                                                return 131.0/248.0;
                                                            } else {
                                                                return 76.0/268.0;
                                                            }
                                                        }
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.num_antecedents_rel <= 0.114798218012f ) {
                                                    return 128.0/214.0;
                                                } else {
                                                    if ( cl->stats.antec_num_total_lits_rel <= 0.154990762472f ) {
                                                        return 77.0/270.0;
                                                    } else {
                                                        return 60.0/290.0;
                                                    }
                                                }
                                            }
                                        } else {
                                            return 176.0/248.0;
                                        }
                                    } else {
                                        return 176.0/190.0;
                                    }
                                }
                            } else {
                                if ( cl->size() <= 9.5f ) {
                                    return 220.0/218.0;
                                } else {
                                    if ( cl->stats.dump_number <= 45.5f ) {
                                        return 145.0/126.0;
                                    } else {
                                        return 251.0/100.0;
                                    }
                                }
                            }
                        } else {
                            if ( cl->size() <= 11.5f ) {
                                if ( cl->size() <= 6.5f ) {
                                    return 139.0/176.0;
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 242777.0f ) {
                                        return 184.0/104.0;
                                    } else {
                                        return 184.0/170.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.633777499199f ) {
                                    return 289.0/202.0;
                                } else {
                                    if ( rdb0_last_touched_diff <= 385851.5f ) {
                                        return 230.0/110.0;
                                    } else {
                                        return 253.0/70.0;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 51215136.0f ) {
                            if ( cl->size() <= 20.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 14.5f ) {
                                    return 110.0/366.0;
                                } else {
                                    if ( cl->stats.dump_number <= 81.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.471811532974f ) {
                                            return 115.0/178.0;
                                        } else {
                                            return 93.0/298.0;
                                        }
                                    } else {
                                        return 133.0/150.0;
                                    }
                                }
                            } else {
                                return 160.0/140.0;
                            }
                        } else {
                            return 119.0/380.0;
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.sum_delta_confl_uip1_used <= 1794.5f ) {
                if ( cl->stats.glue <= 8.5f ) {
                    if ( cl->size() <= 13.5f ) {
                        if ( cl->stats.glue_rel_queue <= 0.807637572289f ) {
                            return 189.0/254.0;
                        } else {
                            return 241.0/144.0;
                        }
                    } else {
                        if ( cl->stats.dump_number <= 16.5f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 4.87413215637f ) {
                                return 203.0/150.0;
                            } else {
                                return 237.0/84.0;
                            }
                        } else {
                            return 260.0/52.0;
                        }
                    }
                } else {
                    if ( cl->stats.glue_rel_queue <= 0.919902741909f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 124234.5f ) {
                            if ( cl->stats.glue_rel_queue <= 0.627586424351f ) {
                                return 154.0/114.0;
                            } else {
                                if ( cl->stats.num_overlap_literals <= 30.5f ) {
                                    return 159.0/98.0;
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 25.4035186768f ) {
                                        if ( cl->size() <= 28.5f ) {
                                            return 267.0/112.0;
                                        } else {
                                            return 305.0/70.0;
                                        }
                                    } else {
                                        return 163.0/82.0;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals <= 105.5f ) {
                                if ( cl->stats.glue_rel_long <= 0.771152973175f ) {
                                    return 242.0/74.0;
                                } else {
                                    if ( cl->size() <= 25.5f ) {
                                        return 188.0/30.0;
                                    } else {
                                        return 201.0/16.0;
                                    }
                                }
                            } else {
                                return 306.0/92.0;
                            }
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 21.039276123f ) {
                            if ( rdb0_last_touched_diff <= 74911.5f ) {
                                if ( cl->stats.sum_uip1_used <= 1.5f ) {
                                    if ( cl->stats.num_overlap_literals <= 134.5f ) {
                                        if ( cl->stats.size_rel <= 0.928508162498f ) {
                                            return 307.0/60.0;
                                        } else {
                                            if ( cl->stats.size_rel <= 1.29804813862f ) {
                                                return 169.0/86.0;
                                            } else {
                                                return 200.0/54.0;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 1.15608727932f ) {
                                            return 172.0/50.0;
                                        } else {
                                            return 346.0/40.0;
                                        }
                                    }
                                } else {
                                    return 235.0/136.0;
                                }
                            } else {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 8.5f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 44.5f ) {
                                        if ( cl->stats.num_overlap_literals <= 203.0f ) {
                                            if ( rdb0_last_touched_diff <= 106937.0f ) {
                                                return 325.0/86.0;
                                            } else {
                                                if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                                    if ( cl->stats.antec_num_total_lits_rel <= 0.899433076382f ) {
                                                        return 338.0/18.0;
                                                    } else {
                                                        return 185.0/30.0;
                                                    }
                                                } else {
                                                    if ( cl->stats.rdb1_last_touched_diff <= 354428.5f ) {
                                                        if ( cl->stats.glue_rel_queue <= 1.18473148346f ) {
                                                            return 256.0/82.0;
                                                        } else {
                                                            return 241.0/38.0;
                                                        }
                                                    } else {
                                                        return 216.0/20.0;
                                                    }
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.glue <= 15.5f ) {
                                                return 309.0/50.0;
                                            } else {
                                                return 389.0/12.0;
                                            }
                                        }
                                    } else {
                                        return 216.0/52.0;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 1.2428920269f ) {
                                        if ( cl->stats.glue_rel_queue <= 1.08878803253f ) {
                                            return 276.0/18.0;
                                        } else {
                                            return 192.0/30.0;
                                        }
                                    } else {
                                        return 322.0/14.0;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 153.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 97837.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 103.5f ) {
                                        return 291.0/42.0;
                                    } else {
                                        return 194.0/86.0;
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 18.5f ) {
                                        return 252.0/12.0;
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 57.5f ) {
                                            return 206.0/46.0;
                                        } else {
                                            return 213.0/14.0;
                                        }
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 25644.0f ) {
                                    return 333.0/80.0;
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 372.263000488f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 70615.5f ) {
                                            if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                                if ( cl->stats.num_total_lits_antecedents <= 739.0f ) {
                                                    if ( cl->stats.size_rel <= 0.940482378006f ) {
                                                        return 233.0/4.0;
                                                    } else {
                                                        return 300.0/54.0;
                                                    }
                                                } else {
                                                    return 265.0/10.0;
                                                }
                                            } else {
                                                return 272.0/50.0;
                                            }
                                        } else {
                                            if ( cl->stats.sum_uip1_used <= 1.5f ) {
                                                if ( cl->stats.num_total_lits_antecedents <= 257.5f ) {
                                                    if ( cl->stats.antec_num_total_lits_rel <= 1.11227846146f ) {
                                                        return 277.0/26.0;
                                                    } else {
                                                        return 234.0/8.0;
                                                    }
                                                } else {
                                                    if ( cl->stats.sum_delta_confl_uip1_used <= 7.5f ) {
                                                        if ( cl->size() <= 109.5f ) {
                                                            if ( cl->stats.dump_number <= 17.5f ) {
                                                                return 274.0/12.0;
                                                            } else {
                                                                return 264.0/2.0;
                                                            }
                                                        } else {
                                                            if ( cl->stats.glue <= 47.5f ) {
                                                                return 214.0/2.0;
                                                            } else {
                                                                return 1;
                                                            }
                                                        }
                                                    } else {
                                                        return 189.0/16.0;
                                                    }
                                                }
                                            } else {
                                                return 389.0/44.0;
                                            }
                                        }
                                    } else {
                                        return 281.0/40.0;
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.rdb1_act_ranking_top_10 <= 4.5f ) {
                    if ( cl->stats.size_rel <= 1.59126091003f ) {
                        if ( cl->stats.sum_uip1_used <= 31.5f ) {
                            if ( cl->stats.sum_uip1_used <= 3.5f ) {
                                if ( cl->stats.glue_rel_long <= 0.848697185516f ) {
                                    return 168.0/198.0;
                                } else {
                                    return 254.0/126.0;
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.627045333385f ) {
                                    return 154.0/360.0;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 50495.5f ) {
                                        if ( cl->stats.size_rel <= 0.808743476868f ) {
                                            return 177.0/346.0;
                                        } else {
                                            return 132.0/192.0;
                                        }
                                    } else {
                                        if ( cl->size() <= 25.5f ) {
                                            return 233.0/148.0;
                                        } else {
                                            return 136.0/160.0;
                                        }
                                    }
                                }
                            }
                        } else {
                            return 125.0/478.0;
                        }
                    } else {
                        return 209.0/106.0;
                    }
                } else {
                    if ( cl->stats.sum_uip1_used <= 3.5f ) {
                        if ( cl->stats.glue <= 16.5f ) {
                            if ( cl->stats.num_overlap_literals_rel <= 0.382229685783f ) {
                                return 167.0/50.0;
                            } else {
                                return 205.0/122.0;
                            }
                        } else {
                            return 219.0/34.0;
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 82905.0f ) {
                            if ( cl->stats.glue_rel_long <= 0.883457303047f ) {
                                return 107.0/260.0;
                            } else {
                                return 139.0/132.0;
                            }
                        } else {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 125562.0f ) {
                                if ( cl->stats.glue <= 13.5f ) {
                                    return 224.0/168.0;
                                } else {
                                    return 234.0/60.0;
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.504545688629f ) {
                                    if ( rdb0_last_touched_diff <= 193775.0f ) {
                                        return 154.0/104.0;
                                    } else {
                                        return 200.0/168.0;
                                    }
                                } else {
                                    return 205.0/272.0;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

static double estimator_should_keep_long_conf4_cluster0_9(
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
    if ( cl->stats.sum_uip1_used <= 4.5f ) {
        if ( cl->stats.antecedents_glue_long_reds_var <= 2.28785705566f ) {
            if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                if ( cl->stats.size_rel <= 0.584081411362f ) {
                    if ( cl->stats.num_overlap_literals_rel <= 0.0231634955853f ) {
                        if ( cl->stats.num_overlap_literals_rel <= 0.00976171437651f ) {
                            return 86.0/274.0;
                        } else {
                            return 106.0/514.0;
                        }
                    } else {
                        if ( cl->stats.dump_number <= 6.5f ) {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 155.5f ) {
                                if ( rdb0_last_touched_diff <= 16522.5f ) {
                                    return 87.0/260.0;
                                } else {
                                    if ( cl->size() <= 8.5f ) {
                                        return 114.0/234.0;
                                    } else {
                                        return 228.0/304.0;
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 12475.5f ) {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                        return 61.0/502.0;
                                    } else {
                                        return 80.0/282.0;
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 22534.5f ) {
                                        return 122.0/226.0;
                                    } else {
                                        return 78.0/270.0;
                                    }
                                }
                            }
                        } else {
                            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                return 154.0/334.0;
                            } else {
                                return 266.0/182.0;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.sum_uip1_used <= 1.5f ) {
                        if ( cl->stats.size_rel <= 0.771144986153f ) {
                            return 135.0/142.0;
                        } else {
                            return 345.0/118.0;
                        }
                    } else {
                        if ( cl->stats.dump_number <= 9.5f ) {
                            if ( cl->stats.sum_uip1_used <= 2.5f ) {
                                return 178.0/190.0;
                            } else {
                                return 196.0/392.0;
                            }
                        } else {
                            return 151.0/124.0;
                        }
                    }
                }
            } else {
                if ( cl->size() <= 11.5f ) {
                    if ( rdb0_last_touched_diff <= 73256.5f ) {
                        if ( cl->stats.glue <= 6.5f ) {
                            if ( cl->stats.size_rel <= 0.0813232660294f ) {
                                return 151.0/178.0;
                            } else {
                                if ( cl->stats.size_rel <= 0.153781995177f ) {
                                    return 99.0/306.0;
                                } else {
                                    if ( cl->stats.dump_number <= 4.5f ) {
                                        if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                                            return 134.0/366.0;
                                        } else {
                                            return 103.0/200.0;
                                        }
                                    } else {
                                        return 223.0/296.0;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals <= 11.5f ) {
                                return 133.0/184.0;
                            } else {
                                return 174.0/182.0;
                            }
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.689429044724f ) {
                            if ( cl->size() <= 5.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0569878816605f ) {
                                    return 203.0/194.0;
                                } else {
                                    return 112.0/184.0;
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 280977.5f ) {
                                    if ( cl->stats.num_overlap_literals <= 6.5f ) {
                                        return 208.0/242.0;
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 168404.0f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 101512.5f ) {
                                                return 165.0/132.0;
                                            } else {
                                                return 237.0/110.0;
                                            }
                                        } else {
                                            return 144.0/130.0;
                                        }
                                    }
                                } else {
                                    return 231.0/112.0;
                                }
                            }
                        } else {
                            return 212.0/140.0;
                        }
                    }
                } else {
                    if ( rdb0_last_touched_diff <= 100321.5f ) {
                        if ( cl->stats.sum_uip1_used <= 2.5f ) {
                            if ( cl->stats.glue_rel_long <= 0.733204483986f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.15993720293f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0394098460674f ) {
                                        return 144.0/158.0;
                                    } else {
                                        return 162.0/288.0;
                                    }
                                } else {
                                    return 247.0/168.0;
                                }
                            } else {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                                    return 199.0/160.0;
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 24.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.136268883944f ) {
                                            return 250.0/178.0;
                                        } else {
                                            return 173.0/74.0;
                                        }
                                    } else {
                                        if ( cl->stats.size_rel <= 0.631269514561f ) {
                                            return 181.0/74.0;
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 57688.5f ) {
                                                return 371.0/56.0;
                                            } else {
                                                return 182.0/50.0;
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 104.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 29803.0f ) {
                                    return 111.0/292.0;
                                } else {
                                    return 204.0/206.0;
                                }
                            } else {
                                return 161.0/106.0;
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_queue <= 0.980510830879f ) {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 161.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 232600.5f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.0546875f ) {
                                        return 318.0/176.0;
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.305359065533f ) {
                                            return 213.0/86.0;
                                        } else {
                                            return 351.0/84.0;
                                        }
                                    }
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 7.5f ) {
                                        if ( cl->size() <= 18.5f ) {
                                            return 218.0/32.0;
                                        } else {
                                            return 201.0/44.0;
                                        }
                                    } else {
                                        return 337.0/34.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 5.5f ) {
                                    return 266.0/84.0;
                                } else {
                                    if ( cl->stats.dump_number <= 31.5f ) {
                                        return 216.0/174.0;
                                    } else {
                                        return 220.0/92.0;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 1.5f ) {
                                if ( cl->stats.glue <= 24.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.648421704769f ) {
                                        return 356.0/46.0;
                                    } else {
                                        return 294.0/14.0;
                                    }
                                } else {
                                    return 184.0/42.0;
                                }
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 146.0f ) {
                                    return 199.0/26.0;
                                } else {
                                    return 162.0/106.0;
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.sum_uip1_used <= 1.5f ) {
                if ( cl->size() <= 13.5f ) {
                    if ( cl->stats.glue_rel_long <= 0.76644384861f ) {
                        if ( cl->stats.num_antecedents_rel <= 0.916310966015f ) {
                            return 158.0/172.0;
                        } else {
                            return 119.0/200.0;
                        }
                    } else {
                        if ( cl->stats.antec_num_total_lits_rel <= 0.50205373764f ) {
                            return 185.0/136.0;
                        } else {
                            if ( rdb0_last_touched_diff <= 38656.5f ) {
                                return 171.0/94.0;
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                    if ( cl->stats.num_overlap_literals <= 131.5f ) {
                                        return 189.0/28.0;
                                    } else {
                                        return 190.0/16.0;
                                    }
                                } else {
                                    return 186.0/50.0;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.glue_rel_queue <= 0.924655795097f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 116500.0f ) {
                            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                return 204.0/142.0;
                            } else {
                                if ( cl->size() <= 18.5f ) {
                                    return 153.0/116.0;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 72711.0f ) {
                                        if ( cl->stats.dump_number <= 4.5f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 18536.0f ) {
                                                return 207.0/40.0;
                                            } else {
                                                return 169.0/96.0;
                                            }
                                        } else {
                                            return 270.0/46.0;
                                        }
                                    } else {
                                        return 233.0/120.0;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 9.19375038147f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 99.5f ) {
                                    return 223.0/72.0;
                                } else {
                                    return 207.0/40.0;
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.820967555046f ) {
                                    return 200.0/34.0;
                                } else {
                                    return 226.0/22.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue <= 13.5f ) {
                            if ( rdb0_last_touched_diff <= 77406.0f ) {
                                if ( rdb0_last_touched_diff <= 22179.5f ) {
                                    return 198.0/98.0;
                                } else {
                                    if ( cl->stats.dump_number <= 5.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 1.06370460987f ) {
                                            return 208.0/78.0;
                                        } else {
                                            return 320.0/34.0;
                                        }
                                    } else {
                                        return 190.0/70.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 8.5f ) {
                                    if ( cl->stats.size_rel <= 0.916168212891f ) {
                                        return 385.0/40.0;
                                    } else {
                                        if ( cl->size() <= 22.5f ) {
                                            return 179.0/58.0;
                                        } else {
                                            return 361.0/46.0;
                                        }
                                    }
                                } else {
                                    return 362.0/18.0;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 8.5f ) {
                                if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                    if ( cl->stats.glue_rel_long <= 1.03740561008f ) {
                                        if ( cl->size() <= 30.5f ) {
                                            return 231.0/18.0;
                                        } else {
                                            if ( cl->stats.glue <= 24.5f ) {
                                                return 205.0/36.0;
                                            } else {
                                                return 187.0/42.0;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 42746.5f ) {
                                            if ( cl->stats.num_antecedents_rel <= 0.795190632343f ) {
                                                if ( cl->stats.glue_rel_long <= 1.19923400879f ) {
                                                    return 277.0/30.0;
                                                } else {
                                                    if ( cl->stats.num_overlap_literals_rel <= 0.496597081423f ) {
                                                        return 199.0/62.0;
                                                    } else {
                                                        return 233.0/44.0;
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.num_total_lits_antecedents <= 572.0f ) {
                                                    if ( cl->stats.num_total_lits_antecedents <= 246.0f ) {
                                                        return 200.0/32.0;
                                                    } else {
                                                        return 215.0/20.0;
                                                    }
                                                } else {
                                                    return 325.0/12.0;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 37.4175338745f ) {
                                                if ( cl->stats.size_rel <= 0.998327851295f ) {
                                                    if ( cl->stats.antecedents_glue_long_reds_var <= 16.8704071045f ) {
                                                        return 246.0/4.0;
                                                    } else {
                                                        return 209.0/6.0;
                                                    }
                                                } else {
                                                    if ( cl->stats.num_overlap_literals <= 146.0f ) {
                                                        return 263.0/46.0;
                                                    } else {
                                                        return 259.0/6.0;
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.num_overlap_literals <= 471.0f ) {
                                                    if ( rdb0_last_touched_diff <= 145868.5f ) {
                                                        return 1;
                                                    } else {
                                                        return 201.0/8.0;
                                                    }
                                                } else {
                                                    return 267.0/16.0;
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 65124.5f ) {
                                        if ( cl->stats.dump_number <= 4.5f ) {
                                            return 302.0/42.0;
                                        } else {
                                            return 242.0/90.0;
                                        }
                                    } else {
                                        if ( cl->stats.glue <= 17.5f ) {
                                            if ( cl->stats.dump_number <= 23.5f ) {
                                                return 266.0/40.0;
                                            } else {
                                                return 173.0/60.0;
                                            }
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 1.3532191515f ) {
                                                if ( cl->stats.size_rel <= 1.31859457493f ) {
                                                    return 342.0/54.0;
                                                } else {
                                                    return 337.0/24.0;
                                                }
                                            } else {
                                                return 380.0/14.0;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 1.23315382004f ) {
                                    if ( rdb0_last_touched_diff <= 142973.0f ) {
                                        return 221.0/10.0;
                                    } else {
                                        if ( cl->stats.size_rel <= 1.24898958206f ) {
                                            return 214.0/2.0;
                                        } else {
                                            return 1;
                                        }
                                    }
                                } else {
                                    return 374.0/24.0;
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.glue_rel_queue <= 0.968020796776f ) {
                    if ( cl->stats.glue <= 9.5f ) {
                        if ( cl->size() <= 9.5f ) {
                            return 87.0/270.0;
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 52338.0f ) {
                                if ( cl->stats.dump_number <= 3.5f ) {
                                    return 125.0/272.0;
                                } else {
                                    return 176.0/210.0;
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                    return 169.0/118.0;
                                } else {
                                    return 303.0/132.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 41278.0f ) {
                            if ( rdb0_last_touched_diff <= 15086.5f ) {
                                return 109.0/232.0;
                            } else {
                                if ( cl->stats.glue <= 14.5f ) {
                                    return 197.0/258.0;
                                } else {
                                    return 191.0/108.0;
                                }
                            }
                        } else {
                            if ( cl->stats.num_antecedents_rel <= 0.872000217438f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 546.5f ) {
                                    return 327.0/80.0;
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.274619519711f ) {
                                        return 176.0/78.0;
                                    } else {
                                        return 184.0/136.0;
                                    }
                                }
                            } else {
                                return 218.0/54.0;
                            }
                        }
                    }
                } else {
                    if ( rdb0_last_touched_diff <= 46365.5f ) {
                        if ( cl->stats.glue_rel_long <= 1.11948513985f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 11.4538822174f ) {
                                return 151.0/150.0;
                            } else {
                                return 204.0/120.0;
                            }
                        } else {
                            if ( cl->stats.size_rel <= 0.989438414574f ) {
                                return 216.0/108.0;
                            } else {
                                return 259.0/64.0;
                            }
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 12.5381946564f ) {
                            if ( cl->stats.glue_rel_queue <= 1.16363692284f ) {
                                if ( cl->size() <= 27.5f ) {
                                    return 232.0/132.0;
                                } else {
                                    return 212.0/42.0;
                                }
                            } else {
                                return 280.0/48.0;
                            }
                        } else {
                            if ( cl->stats.num_antecedents_rel <= 0.774040818214f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 135.0f ) {
                                    return 254.0/62.0;
                                } else {
                                    if ( cl->stats.glue_rel_long <= 1.20207870007f ) {
                                        return 207.0/36.0;
                                    } else {
                                        return 209.0/18.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 1.27197051048f ) {
                                    return 318.0/32.0;
                                } else {
                                    return 1;
                                }
                            }
                        }
                    }
                }
            }
        }
    } else {
        if ( rdb0_act_ranking_top_10 <= 2.5f ) {
            if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                if ( cl->stats.rdb1_last_touched_diff <= 34561.0f ) {
                    if ( cl->stats.glue <= 5.5f ) {
                        if ( cl->stats.sum_uip1_used <= 111.5f ) {
                            if ( cl->stats.size_rel <= 0.4450507164f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 31.5f ) {
                                    if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                                        if ( cl->stats.dump_number <= 44.5f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 11587.0f ) {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.0891632288694f ) {
                                                    if ( cl->stats.glue_rel_long <= 0.260639727116f ) {
                                                        return 60.0/322.0;
                                                    } else {
                                                        if ( cl->stats.glue_rel_long <= 0.397676229477f ) {
                                                            return 41.0/434.0;
                                                        } else {
                                                            return 60.0/430.0;
                                                        }
                                                    }
                                                } else {
                                                    if ( cl->stats.antec_num_total_lits_rel <= 0.13045334816f ) {
                                                        return 30.0/462.0;
                                                    } else {
                                                        return 36.0/318.0;
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.num_overlap_literals_rel <= 0.0520893223584f ) {
                                                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                                        return 60.0/300.0;
                                                    } else {
                                                        return 86.0/254.0;
                                                    }
                                                } else {
                                                    return 62.0/424.0;
                                                }
                                            }
                                        } else {
                                            return 104.0/334.0;
                                        }
                                    } else {
                                        if ( cl->stats.size_rel <= 0.200699925423f ) {
                                            if ( cl->stats.glue <= 3.5f ) {
                                                return 58.0/370.0;
                                            } else {
                                                return 50.0/446.0;
                                            }
                                        } else {
                                            return 44.0/646.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.35266906023f ) {
                                        return 78.0/278.0;
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 30.5f ) {
                                            return 79.0/294.0;
                                        } else {
                                            return 51.0/340.0;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 8638.5f ) {
                                    return 77.0/398.0;
                                } else {
                                    return 127.0/310.0;
                                }
                            }
                        } else {
                            if ( cl->stats.num_antecedents_rel <= 0.0406155511737f ) {
                                return 44.0/332.0;
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 19.5f ) {
                                    if ( rdb0_last_touched_diff <= 15611.0f ) {
                                        if ( cl->stats.glue_rel_long <= 0.316186189651f ) {
                                            return 20.0/420.0;
                                        } else {
                                            return 11.0/584.0;
                                        }
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0832650363445f ) {
                                            return 15.0/416.0;
                                        } else {
                                            return 33.0/408.0;
                                        }
                                    }
                                } else {
                                    return 60.0/592.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 3977069.0f ) {
                            if ( cl->stats.sum_uip1_used <= 13.5f ) {
                                if ( cl->stats.dump_number <= 19.5f ) {
                                    if ( cl->stats.glue <= 14.5f ) {
                                        if ( cl->size() <= 12.5f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.122161611915f ) {
                                                if ( cl->stats.dump_number <= 4.5f ) {
                                                    return 67.0/328.0;
                                                } else {
                                                    return 85.0/274.0;
                                                }
                                            } else {
                                                return 97.0/550.0;
                                            }
                                        } else {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.624365329742f ) {
                                                if ( cl->stats.glue_rel_queue <= 0.661109745502f ) {
                                                    return 201.0/436.0;
                                                } else {
                                                    if ( cl->stats.glue_rel_queue <= 0.793477773666f ) {
                                                        return 69.0/276.0;
                                                    } else {
                                                        return 160.0/362.0;
                                                    }
                                                }
                                            } else {
                                                return 81.0/292.0;
                                            }
                                        }
                                    } else {
                                        return 210.0/402.0;
                                    }
                                } else {
                                    return 234.0/296.0;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 40.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.391215622425f ) {
                                        return 96.0/408.0;
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.472604185343f ) {
                                            return 48.0/530.0;
                                        } else {
                                            if ( cl->stats.sum_uip1_used <= 17.5f ) {
                                                return 125.0/538.0;
                                            } else {
                                                if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                                                    if ( cl->stats.sum_delta_confl_uip1_used <= 2114246.5f ) {
                                                        if ( cl->stats.num_antecedents_rel <= 0.216534867883f ) {
                                                            return 110.0/689.9;
                                                        } else {
                                                            return 45.0/414.0;
                                                        }
                                                    } else {
                                                        return 106.0/332.0;
                                                    }
                                                } else {
                                                    return 49.0/612.0;
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                                        return 134.0/392.0;
                                    } else {
                                        return 128.0/520.0;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 2.03111124039f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.0643233656883f ) {
                                    return 68.0/318.0;
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 20243416.0f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.182912066579f ) {
                                            if ( cl->stats.glue_rel_queue <= 0.558810591698f ) {
                                                return 63.0/462.0;
                                            } else {
                                                return 26.0/354.0;
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 11035.5f ) {
                                                return 77.0/466.0;
                                            } else {
                                                return 82.0/226.0;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.dump_number <= 56.5f ) {
                                            return 28.0/482.0;
                                        } else {
                                            if ( cl->stats.sum_uip1_used <= 150.0f ) {
                                                return 54.0/346.0;
                                            } else {
                                                return 39.0/422.0;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 57.0f ) {
                                    return 70.0/302.0;
                                } else {
                                    return 77.0/488.0;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->size() <= 26.5f ) {
                        if ( cl->stats.sum_uip1_used <= 76.5f ) {
                            if ( rdb0_last_touched_diff <= 5624.0f ) {
                                return 90.0/360.0;
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.365884512663f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.817810297012f ) {
                                        if ( cl->stats.dump_number <= 31.5f ) {
                                            if ( cl->size() <= 15.5f ) {
                                                if ( cl->stats.size_rel <= 0.240522444248f ) {
                                                    return 159.0/328.0;
                                                } else {
                                                    if ( cl->stats.num_antecedents_rel <= 0.177622869611f ) {
                                                        return 49.0/332.0;
                                                    } else {
                                                        return 76.0/264.0;
                                                    }
                                                }
                                            } else {
                                                return 131.0/240.0;
                                            }
                                        } else {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0765752717853f ) {
                                                return 116.0/230.0;
                                            } else {
                                                return 118.0/150.0;
                                            }
                                        }
                                    } else {
                                        return 116.0/176.0;
                                    }
                                } else {
                                    return 180.0/300.0;
                                }
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 193.5f ) {
                                return 102.0/468.0;
                            } else {
                                return 68.0/480.0;
                            }
                        }
                    } else {
                        if ( cl->stats.num_overlap_literals <= 88.5f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 2.23611116409f ) {
                                return 155.0/304.0;
                            } else {
                                return 165.0/166.0;
                            }
                        } else {
                            return 193.0/132.0;
                        }
                    }
                }
            } else {
                if ( cl->stats.used_for_uip_creation <= 5.5f ) {
                    if ( cl->stats.rdb1_act_ranking_top_10 <= 4.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 15206.5f ) {
                            if ( cl->stats.num_overlap_literals_rel <= 0.221371993423f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0159449819475f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 14.5f ) {
                                        return 34.0/568.0;
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.419822692871f ) {
                                            return 35.0/360.0;
                                        } else {
                                            return 51.0/324.0;
                                        }
                                    }
                                } else {
                                    if ( cl->size() <= 10.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.067853808403f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 4146.5f ) {
                                                if ( cl->stats.size_rel <= 0.149398416281f ) {
                                                    return 25.0/604.0;
                                                } else {
                                                    if ( cl->stats.sum_uip1_used <= 47.5f ) {
                                                        return 12.0/400.0;
                                                    } else {
                                                        return 11.0/693.9;
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.size_rel <= 0.206157565117f ) {
                                                    return 49.0/620.0;
                                                } else {
                                                    return 18.0/620.0;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                                if ( cl->stats.num_antecedents_rel <= 0.192394167185f ) {
                                                    return 39.0/320.0;
                                                } else {
                                                    return 29.0/452.0;
                                                }
                                            } else {
                                                if ( cl->stats.num_antecedents_rel <= 0.206263005733f ) {
                                                    return 30.0/526.0;
                                                } else {
                                                    return 13.0/496.0;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 10.5f ) {
                                            if ( cl->stats.num_total_lits_antecedents <= 30.5f ) {
                                                return 51.0/582.0;
                                            } else {
                                                return 24.0/462.0;
                                            }
                                        } else {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.110788218677f ) {
                                                return 51.0/312.0;
                                            } else {
                                                if ( cl->stats.num_total_lits_antecedents <= 67.5f ) {
                                                    return 40.0/540.0;
                                                } else {
                                                    return 71.0/556.0;
                                                }
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.90154081583f ) {
                                    if ( cl->stats.size_rel <= 0.479128152132f ) {
                                        return 38.0/606.0;
                                    } else {
                                        return 81.0/514.0;
                                    }
                                } else {
                                    return 71.0/322.0;
                                }
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals <= 50.5f ) {
                                if ( cl->stats.sum_uip1_used <= 37.5f ) {
                                    if ( rdb0_last_touched_diff <= 6145.0f ) {
                                        if ( cl->size() <= 12.5f ) {
                                            return 73.0/540.0;
                                        } else {
                                            return 60.0/276.0;
                                        }
                                    } else {
                                        return 80.0/300.0;
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 36.5f ) {
                                        return 32.0/550.0;
                                    } else {
                                        return 48.0/374.0;
                                    }
                                }
                            } else {
                                return 82.0/232.0;
                            }
                        }
                    } else {
                        return 133.0/476.0;
                    }
                } else {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                        if ( cl->stats.glue_rel_queue <= 0.464756309986f ) {
                            if ( cl->stats.glue_rel_long <= 0.313829958439f ) {
                                if ( cl->stats.sum_uip1_used <= 46.5f ) {
                                    return 57.0/550.0;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 10674.5f ) {
                                        return 11.0/558.0;
                                    } else {
                                        return 27.0/376.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.422210335732f ) {
                                    if ( rdb0_last_touched_diff <= 2898.5f ) {
                                        return 14.0/677.9;
                                    } else {
                                        return 16.0/386.0;
                                    }
                                } else {
                                    return 28.0/466.0;
                                }
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 7.04938268661f ) {
                                if ( cl->stats.glue_rel_long <= 0.481912016869f ) {
                                    return 52.0/294.0;
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 20.5f ) {
                                        if ( cl->stats.sum_uip1_used <= 52.5f ) {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 1092981.0f ) {
                                                return 37.0/596.0;
                                            } else {
                                                return 54.0/368.0;
                                            }
                                        } else {
                                            if ( cl->stats.size_rel <= 0.40523031354f ) {
                                                return 19.0/614.0;
                                            } else {
                                                return 28.0/376.0;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.743410348892f ) {
                                            return 30.0/484.0;
                                        } else {
                                            return 15.0/500.0;
                                        }
                                    }
                                }
                            } else {
                                return 42.0/348.0;
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 7846.0f ) {
                            if ( cl->stats.num_overlap_literals <= 25.5f ) {
                                if ( cl->stats.glue_rel_queue <= 0.162381887436f ) {
                                    return 32.0/616.0;
                                } else {
                                    if ( cl->stats.size_rel <= 0.396850526333f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 1112.5f ) {
                                            if ( cl->stats.size_rel <= 0.0921099632978f ) {
                                                if ( cl->stats.glue_rel_long <= 0.356251984835f ) {
                                                    return 9.0/580.0;
                                                } else {
                                                    return 14.0/410.0;
                                                }
                                            } else {
                                                if ( cl->stats.dump_number <= 2.5f ) {
                                                    return 9.0/546.0;
                                                } else {
                                                    if ( cl->stats.sum_delta_confl_uip1_used <= 5399729.5f ) {
                                                        return 6.0/606.0;
                                                    } else {
                                                        if ( cl->stats.used_for_uip_creation <= 13.5f ) {
                                                            return 4.0/430.0;
                                                        } else {
                                                            return 0.0/1411.9;
                                                        }
                                                    }
                                                }
                                            }
                                        } else {
                                            if ( rdb0_last_touched_diff <= 725.5f ) {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.0415106378496f ) {
                                                    return 3.0/416.0;
                                                } else {
                                                    return 1.0/771.9;
                                                }
                                            } else {
                                                if ( cl->size() <= 4.5f ) {
                                                    return 24.0/522.0;
                                                } else {
                                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0847841799259f ) {
                                                        if ( cl->stats.glue_rel_long <= 0.40034109354f ) {
                                                            return 26.0/514.0;
                                                        } else {
                                                            return 6.0/422.0;
                                                        }
                                                    } else {
                                                        return 5.0/681.9;
                                                    }
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.used_for_uip_creation <= 25.5f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 2421.5f ) {
                                                if ( cl->stats.num_overlap_literals_rel <= 0.0657189041376f ) {
                                                    return 24.0/352.0;
                                                } else {
                                                    return 10.0/476.0;
                                                }
                                            } else {
                                                return 29.0/358.0;
                                            }
                                        } else {
                                            return 14.0/721.9;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 1.12046480179f ) {
                                    if ( cl->stats.glue <= 6.5f ) {
                                        return 7.0/590.0;
                                    } else {
                                        if ( cl->stats.sum_uip1_used <= 139.5f ) {
                                            return 22.0/360.0;
                                        } else {
                                            return 5.0/434.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 84.5f ) {
                                        return 48.0/452.0;
                                    } else {
                                        return 28.0/478.0;
                                    }
                                }
                            }
                        } else {
                            return 50.0/346.0;
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.size_rel <= 0.726196169853f ) {
                if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                    if ( cl->stats.rdb1_last_touched_diff <= 29247.5f ) {
                        if ( cl->stats.glue <= 7.5f ) {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 4.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 22.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 10.5f ) {
                                        return 77.0/296.0;
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.489234745502f ) {
                                            return 71.0/502.0;
                                        } else {
                                            return 36.0/388.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.565199971199f ) {
                                        return 105.0/526.0;
                                    } else {
                                        return 96.0/302.0;
                                    }
                                }
                            } else {
                                return 76.0/717.9;
                            }
                        } else {
                            if ( cl->size() <= 24.5f ) {
                                if ( cl->stats.sum_uip1_used <= 14.5f ) {
                                    return 100.0/236.0;
                                } else {
                                    return 67.0/280.0;
                                }
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 768935.0f ) {
                                    return 94.0/376.0;
                                } else {
                                    return 44.0/324.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.sum_uip1_used <= 24.5f ) {
                            if ( cl->stats.glue_rel_queue <= 0.523408055305f ) {
                                if ( cl->stats.sum_uip1_used <= 12.5f ) {
                                    return 167.0/134.0;
                                } else {
                                    return 137.0/174.0;
                                }
                            } else {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.207040816545f ) {
                                    return 95.0/202.0;
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 2.07999992371f ) {
                                        return 134.0/144.0;
                                    } else {
                                        return 128.0/174.0;
                                    }
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 84952.5f ) {
                                if ( cl->stats.glue <= 6.5f ) {
                                    if ( cl->stats.dump_number <= 32.5f ) {
                                        return 51.0/324.0;
                                    } else {
                                        return 71.0/338.0;
                                    }
                                } else {
                                    return 94.0/278.0;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0647223591805f ) {
                                    return 115.0/216.0;
                                } else {
                                    return 78.0/254.0;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_last_touched_diff <= 62555.0f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 27717.5f ) {
                            if ( cl->stats.glue_rel_long <= 0.663170933723f ) {
                                if ( cl->stats.glue_rel_queue <= 0.418702244759f ) {
                                    return 50.0/400.0;
                                } else {
                                    return 50.0/318.0;
                                }
                            } else {
                                return 85.0/306.0;
                            }
                        } else {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 426525.5f ) {
                                if ( cl->stats.dump_number <= 7.5f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0804986059666f ) {
                                        return 87.0/234.0;
                                    } else {
                                        return 90.0/210.0;
                                    }
                                } else {
                                    return 156.0/256.0;
                                }
                            } else {
                                if ( cl->stats.sum_uip1_used <= 40.5f ) {
                                    return 95.0/378.0;
                                } else {
                                    return 44.0/348.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 187763.0f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 7.5f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 4462741.5f ) {
                                    if ( cl->stats.sum_uip1_used <= 28.5f ) {
                                        if ( rdb0_last_touched_diff <= 89795.0f ) {
                                            return 212.0/176.0;
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 127446.0f ) {
                                                if ( cl->stats.dump_number <= 20.5f ) {
                                                    return 180.0/388.0;
                                                } else {
                                                    return 122.0/186.0;
                                                }
                                            } else {
                                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.283922374249f ) {
                                                    return 146.0/188.0;
                                                } else {
                                                    return 156.0/132.0;
                                                }
                                            }
                                        }
                                    } else {
                                        return 133.0/318.0;
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 46.5f ) {
                                        return 57.0/276.0;
                                    } else {
                                        return 117.0/184.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.dump_number <= 24.5f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 249675.5f ) {
                                        return 178.0/300.0;
                                    } else {
                                        return 65.0/316.0;
                                    }
                                } else {
                                    return 207.0/398.0;
                                }
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 54.5f ) {
                                if ( cl->stats.size_rel <= 0.133262097836f ) {
                                    return 152.0/164.0;
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 21.5f ) {
                                        if ( rdb0_last_touched_diff <= 283124.5f ) {
                                            if ( cl->size() <= 12.5f ) {
                                                return 163.0/204.0;
                                            } else {
                                                return 179.0/134.0;
                                            }
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.271856635809f ) {
                                                return 345.0/140.0;
                                            } else {
                                                return 180.0/112.0;
                                            }
                                        }
                                    } else {
                                        if ( cl->size() <= 11.5f ) {
                                            return 126.0/164.0;
                                        } else {
                                            return 164.0/126.0;
                                        }
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 383075.5f ) {
                                    return 173.0/282.0;
                                } else {
                                    return 77.0/254.0;
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.sum_uip1_used <= 38.5f ) {
                    if ( cl->stats.rdb1_last_touched_diff <= 59613.5f ) {
                        if ( cl->stats.glue_rel_queue <= 0.687100529671f ) {
                            return 140.0/374.0;
                        } else {
                            if ( cl->stats.num_antecedents_rel <= 0.730734169483f ) {
                                if ( rdb0_last_touched_diff <= 39728.0f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 91.5f ) {
                                        return 103.0/258.0;
                                    } else {
                                        return 121.0/176.0;
                                    }
                                } else {
                                    return 194.0/248.0;
                                }
                            } else {
                                return 160.0/122.0;
                            }
                        }
                    } else {
                        if ( cl->stats.num_antecedents_rel <= 0.641147613525f ) {
                            if ( rdb0_last_touched_diff <= 192950.5f ) {
                                if ( cl->stats.sum_uip1_used <= 6.5f ) {
                                    return 164.0/94.0;
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                        return 194.0/132.0;
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.843645572662f ) {
                                            return 140.0/150.0;
                                        } else {
                                            return 112.0/194.0;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->size() <= 40.5f ) {
                                    return 277.0/158.0;
                                } else {
                                    return 202.0/56.0;
                                }
                            }
                        } else {
                            if ( cl->stats.glue <= 12.5f ) {
                                return 173.0/108.0;
                            } else {
                                return 229.0/50.0;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_last_touched_diff <= 59930.0f ) {
                        return 61.0/396.0;
                    } else {
                        return 159.0/306.0;
                    }
                }
            }
        }
    }
}

static bool should_keep_long_conf4_cluster0(
    const CMSat::Clause* cl
    , const uint64_t sumConflicts
    , const uint32_t rdb0_last_touched_diff
    , const uint32_t rdb0_act_ranking
    , const uint32_t rdb0_act_ranking_top_10
) {
    int votes = 0;
    votes += estimator_should_keep_long_conf4_cluster0_0(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_long_conf4_cluster0_1(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_long_conf4_cluster0_2(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_long_conf4_cluster0_3(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_long_conf4_cluster0_4(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_long_conf4_cluster0_5(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_long_conf4_cluster0_6(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_long_conf4_cluster0_7(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_long_conf4_cluster0_8(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_long_conf4_cluster0_9(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    return votes >= 5;
}
}
