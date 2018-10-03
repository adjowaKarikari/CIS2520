var fileArray;
var fileName;
var responseData;
$('.panel').click(function (e)  {

    if(e.currentTarget.id=="panelFileLog")
    {
        prepareRequest("getFileLog",null,"GET");
        var data=JSON.parse(responseData);
            for(var i in data){
                var rowData =data[i];
                var rowStr = "<tr>"
                    + "<td><a href="+rowData.Name+">Download</a></td>"
                    + "<td>"+rowData.Source+"</td>"
                    + "<td>"+rowData.GEDCVersion+"</td>"
                    + "<td>"+rowData.Encoding+"</td>"
                    + "<td>"+rowData.SubmitterName+"</td>"
                    + "<td>"+rowData.SubmitterAddress+"</td>"
                    + "<td>"+rowData.NumberofIndividuals+"</td>"
                    + "<td>"+rowData.NumberofFamilies+"</td>"
                    + "</tr>"
                $("#tblFileLog tbody").append(rowStr);
            }
    }
    else if(e.currentTarget.id=="panelGEDCOM")
    {
        prepareRequest("getGEDCOM",null,"GET");
        var data=JSON.parse(responseData);
        for(var i in data){
            var rowData =data[i];
            var rowStr = "<tr>"
                + "<td>"+rowData.GivenName+"</td>"
                + "<td>"+rowData.Surname+"</td>"
                + "<td>"+rowData.Sex+"</td>"
                + "<td>"+rowData.FamilySize+"</td>"
                + "</tr>"
            $("#tblgedCOM tbody").append(rowStr);
        }
    }
    else if(e.currentTarget.id=='panelIndividual')
    {
        $('#uploadFile').css('visibility', 'hidden');

        if($('#ddlcas').children('option').length==1){
            prepareRequest("getGEDCOMFileName",null,"GET");
            var data=JSON.parse(responseData);
            $.each(data, function(i, p) {
                $('#ddlcas').append($('<option></option>').val(p).html(p));
            });
        }


    }

    else if(e.currentTarget.id=='panelDatabase'){
        $('#divDatabase').css('visibility','hidden');
    }
    else if(e.currentTarget.id=='panelStoreFile'){
        prepareRequest("getFilesData",null,"GET");
        $("#tblFiles tbody").empty();
        var data=JSON.parse(responseData);

        for(var i in data){
            var rowData =data[i];
            var rowStr = "<tr>"
                + "<td>"+rowData["file_id"]+"</td>"
                + "<td>"+rowData["file_Name"]+"</td>"
                + "<td>"+rowData["source"]+"</td>"
                + "<td>"+rowData["version"]+"</td>"
                + "<td>"+rowData["encoding"]+"</td>"
                + "<td>"+rowData["sub_name"]+"</td>"
                + "<td>"+rowData["sub_addr"]+"</td>"
                + "<td>"+rowData["num_individials"]+"</td>"
                + "<td>"+rowData["num_families"]+"</td>"
                + "</tr>"
            $("#tblFiles tbody").append(rowStr);
        }
        prepareRequest("getIndividualsData",null,"GET");
        $("#tblIndividual tbody").empty();
        var individualData=JSON.parse(responseData);
        for(var i in individualData){
            var rowData =individualData[i];
            var rowStr = "<tr>"
                + "<td>"+rowData["ind_id"]+"</td>"
                + "<td>"+rowData["given_name"]+"</td>"
                + "<td>"+rowData["surname"]+"</td>"
                + "<td>"+rowData["sex"]+"</td>"
                + "<td>"+rowData["fam_size"]+"</td>"
                + "<td>"+rowData["source_file"]+"</td>"
                + "</tr>"
            $("#tblIndividual tbody").append(rowStr);
        }
    }
    else
        {
            return;
        }
});

function json2array(json){
    var result = [];
    var keys = Object.keys(json);
    keys.forEach(function(key){
        result.push(json[key]);
    });
    return result;
}

function connectDB() {
    prepareRequest("connectDB",{ dbName: $('#txtDB').val(),user: $('#txtUser').val(),pass: $('#txtPass').val()},"POST");
    if(responseData=="Ok"){
        $('#divDatabase').css('visibility','visible');
    }
    else{
        $('#collapseFive').find('#authButton').append('<span>DB Informations Are Fault!!</span><br>');

    }
}

function uploadGedFile(e) {
    var file = e.files[0]; //Files[0] = 1st file
    var reader = new FileReader();
    reader.readAsText(file, 'UTF-8');
    //reader.onload = fileRenderer();
    reader.onload= function () {
        var arrayBuffer = this.result;
            array = new Uint8Array(arrayBuffer);
        fileName=file.name;
        fileArray=array;
    }
    reader.readAsArrayBuffer(e.files[0]);

    prepareRequest("upload",{ data: fileArray, name: fileName },"POST");
}

    // Event listener form replacement example, building a Single-Page-App, no redirects if possible
    $('#someform').submit(function(e){
        e.preventDefault();
        $.ajax({});
    });


function runQuery() {
    prepareRequest("executeQuery",{ data: $('#txtQuery').val()},"POST");
    $('#collapseFive').find('#queryResult').append('<span>'+responseData+'</span><br>');

}

function getDBStatus() {
   prepareRequest("getFileStatus",null,"GET");
    var fileCount=JSON.parse(responseData);
   prepareRequest("getIndividualStatus",null,"GET");
    var individualCount=JSON.parse(responseData);
    $('#collapseFive').find('#divFile').append('<span>Database has '+fileCount['count(file_id)']+' files and '+individualCount['count(ind_id)']+' individuals</span><br>');
}

function clearAllData(){
    prepareRequest("clearData",null,"GET");
    $('#collapseFive').find('#divClearAllData').append('<span>Result: '+responseData+'</span><br>');
}

function prepareRequest(methodName,data, requestType){
        $('#collapseOne').find('.panel-body').append('<br><span>'+ methodName+' is requested</span><br>');
        var url = "http://127.0.0.1:8000/" + methodName;
        var method = requestType;
        var postData = JSON.stringify(data);

        var shouldBeAsync = true;

        var request = new XMLHttpRequest();

        request.onload = function () {
            var status = request.status; // HTTP response status, e.g., 200 for "200 OK"
            if(status==200){
                //console.log(data);
                $('#collapseOne').find('.panel-body').append('<span>'+ methodName + ' result is ' + request.responseText+'</span>');
                responseData=null;
                responseData=request.responseText;
                return request.responseText;
            }
            else
            {
                $('#collapseOne').find('.panel-body').append('<span>'+ request.responseText+'</span>');
            }
        }

        request.open(method, url,false);
        request.setRequestHeader('Accept', 'application/json');
        request.setRequestHeader('Content-Type', 'application/json');
        request.setRequestHeader('Access-Control-Allow-Origin', 'http://127.0.0.1:8000');
        request.setRequestHeader('Access-Control-Allow-Credentials', 'true');
        request.setRequestHeader('Access-Control-Allow-Methods', 'GET,PUT,POST,DELETE,OPTIONS');
        request.setRequestHeader('Access-Control-Allow-Headers', 'Content-Type,Authorization');
    if(methodName=="upload" || methodName=="executeQuery" || methodName=="connectDB")
    {
        request.send(postData);
    }
    else {
        request.send();
    }
    }


