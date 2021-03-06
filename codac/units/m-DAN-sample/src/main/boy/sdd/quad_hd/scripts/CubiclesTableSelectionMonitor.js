importClass(Packages.org.csstudio.opibuilder.scriptUtil.DataUtil);
importClass(Packages.org.csstudio.opibuilder.scriptUtil.PVUtil);
importClass(Packages.org.csstudio.opibuilder.scriptUtil.ScriptUtil);
importClass(Packages.org.csstudio.opibuilder.scriptUtil.ConsoleUtil);

var table = widget.getTable();
var fct_name=display.getPropertyValue("name");
var selectionChanged  = new Packages.org.csstudio.swt.widgets.natives.SpreadSheetTable.ITableSelectionChangedListener() {
    selectionChanged: function(selection) {
 		
    	var selectedrow=  table.getSelection();
    	var cuIndex=selectedrow[0][0];
		var phyName=selectedrow[0][1];

		var macroInput = DataUtil.createMacrosInput(true)
		macroInput.put("CUB", cuIndex)
		macroInput.put("PHY_NAME", phyName)
		macroInput.put("FCT_NAME", fct_name)
        		
		ScriptUtil.openOPI(display.getWidget("Table"), fct_name+"-CubicleDetails.opi", 1, macroInput);
	}
};
table.addSelectionChangedListener(selectionChanged);
