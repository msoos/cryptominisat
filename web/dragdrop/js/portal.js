// jsportal (iGoogle-like drag&drop portal v2.0, Mon Sep 14 09:00:00 +0100 2008

// Copyright (c) 2009 Michel Hiemstra (http://michelhiemstra.nl)

// jsportal v2.0 is freely distributable under the terms of an Creative Commons license.
// For details, see the authors web site: http://michelhiemstra/

if(typeof(Prototype) == "undefined")
	throw "Portal requires Prototype to be loaded.";
if(typeof(Effect) == "undefined")
	throw "Portal requires Effects to be loaded.";
if(typeof(Sortable) == "undefined")
	throw "Portal requires Sortable to be loaded.";

var Portal = Class.create();

Portal.prototype = {

	initialize : function (settings, options, data) {

		// set options
		this.setOptions(options);
		
		// set blocks to their positions
		this.apply_settings(settings);
		
		// load data to blocks
		//this.loadData(data);
						
		// if editor is enabled we proceed
		if (!this.options.editorEnabled) return;
		
		// get all available columns
		var columns = $(this.options.portal).getElementsByClassName(this.options.column);
		
		// loop trough columns array
		$A(columns).each(function(column) {
			
			// create sortable
			Sortable.create(column, {
				containment : $A(columns),
				constraint  : this.options.constraint,
				ghosting	: this.options.ghosting,
				tag 		: this.options.tag,
				only 		: this.options.block,
				dropOnEmpty : this.options.droponempty,
				handle 		: this.options.handle,
				hoverclass 	: this.options.hoverclass,
				scroll		: this.options.scroll,
				onUpdate 	: function (container) {
					
					// if we dont have a save url we dont update
					if (!this.options.saveurl) return;
					
					// if we are in the same container we do nothing
					if (container.id == this.options.blocklist) return;
					
					// get blocks in this container
					var blocks = container.getElementsByClassName(this.options.block);
					
					// serialize all blocks in this container
					var postBody = container.id + ':';
					postBody += $A(blocks).pluck('id').join(',');
					postBody = 'value=' + escape(postBody);
					
					// save it to the database
					new Ajax.Request(this.options.saveurl, { method: 'post', postBody: postBody, onComplete : function (t) {
						$('data').update(t.responseText + $('data').innerHTML);
					} });
										
				}.bind(this)
			});
			
		}.bind(this));
	},
	
	apply_settings : function (settings) {
		// apply settings to the array
		for (var container in settings) {
			settings[container].each(function (block) { if (!$(container)) { console.log('Block '+container+' not found') } else { $(container).appendChild($(block)); }  });
		}
	},
	
	setOptions : function (options) {
		// set options
		this.options = {
			tag				: 'div',
			editorEnabled 	: false,
			portal			: 'portal',
			column			: 'column',
			block			: 'block',
			content			: 'content',
			configElement	: 'config',
			configSave		: 'save-button',
			configCancel	: 'cancel-button',
			configSaved		: 'config-saved',
			handle			: 'draghandle',
			hoverclass		: 'target',
			scroll			: window,
			remove			: 'option-remove',
			config			: 'option-edit',
			blocklist		: 'portal-column-block-list',
			saveurl			: false,
			constraint		: false,
			ghosting		: false,
			droponempty		: true
		}
		
		Object.extend(this.options, options || {});
	},
	
	loadData : function (data) {
		// load data for each block
		for (var type in data) {
			data[type].each(function(block) {
				for (var blockname in block) {
					
					switch (type) {
						
						default:
							new Ajax.Updater(blockname + '-content', '/module/'+type+'/data='+block[blockname], { evalScripts : true });
					}
				}
			}.bind(this));
		}
	}
};