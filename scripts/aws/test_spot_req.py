#!/usr/bin/env python
# -*- coding: utf-8 -*-

import RequestSpotClient

#create spot instance requests
spot_create = RequestSpotClient.RequestSpotClient()
spot_create.create_spots()
