var express = require('express');
var cors = require('cors');
var fs = require('fs');
var mysql = require('mysql');
var bodyParser = require('body-parser');
var app = express();

var dbStatus=[];

var sqlString="";
var user='root';
var database="audreydb";
var password="user123**";

//mysql connection options
var connection = mysql.createConnection({
    host     : 'localhost',
    user     : 'root',
    port     : '3306',
    password : 'user123**',
    database : 'audreydb'
});

app.use(cors());
app.use(bodyParser.json()); // support json encoded bodies
app.use(bodyParser.urlencoded({ extended: true })); // support encoded bodies

app.get('/', function (req, res) {
    res.send('Hello World!')
});

app.listen(8000, function () {
    console.log('Example app listening on port 8000!')
});


app.post('/connectDB',function (req,res){

    console.log(req.body.data);
    if(req.body.dbName==database&&req.body.user==user&&req.body.pass==password){
        return res.status(200).send("Ok");
    }
    else
        {
        return res.send("NotOk");
    }

});

//Respond to POST requests that upload files to uploads/ directory
app.post('/upload', function(req, res) {

        // write method to upload file in c language and define return data
        //console.log(req.body.data);
        var fileData={};
        fileData.file_name="deneme";
        fileData.source="deneme";
        fileData.version="1";
        fileData.encoding="1";
        fileData.sub_name="1";
        fileData.sub_addr="1";
        fileData.num_individuals=0;
        fileData.num_families=0;

        var individualsData={};
        individualsData.sex="m";
        individualsData.surname="k";
        individualsData.given_name="k";
        individualsData.fam_size=0;

    var fileInsert='INSERT INTO file(file_name,source,version,encoding,sub_name,sub_addr,num_individials,num_families) VALUES(?,?,?,?,?,?,?,?);';
    var fileData=connection.query(fileInsert,[fileData.file_name,fileData.source,fileData.version,fileData.encoding,fileData.sub_name,fileData.sub_addr,fileData.num_individuals,fileData.num_families]);

    fileData.on('result', function(result) {
        var individialQuery='INSERT INTO individual(surname,given_name,sex,fam_size,source_file) VALUES(?,?,?,?,?);';
        connection.query(individialQuery,[individualsData.surname,individualsData.given_name,individualsData.sex,individualsData.fam_size,result.insertId]);

    });


    res.status(200).send('File is uploaded.');

    // connection.connect(function(err) {
    //  if (err) throw err
    //
    //     var fileInsert='INSERT INTO file(file_name,source,version,encoding,sub_name,sub_addr,num_individials,num_families) ' +
    //     'VALUES(?,?,?,?,?,?,?,?);';
    //
    //
    //  connection.query(fileInsert,[fileData.file_name,fileData.source,fileData.version,fileData.encoding,fileData.sub_name,
    //      fileData.sub_addr,fileData.num_individuals,fileData.num_families], function(err, result) {
    //  if (err){
    //      console.log(err);
    //      res.send(err);
    //  }
    //  else{
    //      console.log(result);
    //         var individialQuery='INSERT INTO individual(surname,given_name,sex,fam_size,source_file) VALUES(?,?,?,?,?);';
    //      connection.query(individialQuery,[individualsData.surname,individualsData.given_name,individualsData.sex,individualsData.fam_size,result.insertId],function (error,success){
    //          if (error){
    //              console.log(error);
    //              res.send(error);
    //          }
    //          console.log(success);
    //      });
    //      console.log(result);
    //  }
    //  });
    //  });

    //res.status(200).send('File is uploaded. File includes these data; '+ JSON.stringify(fileData) +' '+JSON.stringify(individualsData));
    //return res.status(200).send('File is uploaded. File includes these data; '+ JSON.stringify(returnData));
});

app.post('/executeQuery', function(req, res) {

    sqlString=req.body.data;
    executeQuery(function (result){
        //you might want to do something is err is not null...
        /*  res.render('SQLtest', { 'title': 'SQL test',
         'result': result});*/
        console.log(result);
        res.status(200).send(JSON.stringify(result));
    });
});

app.get('/getFileStatus', function(req, res) {
    getFileCounts(function (result){
        //you might want to do something is err is not null...
      /*  res.render('SQLtest', { 'title': 'SQL test',
            'result': result});*/
      console.log(result);
        res.status(200).send(JSON.stringify(result));
    });
});

app.get('/getIndividualStatus', function(req, res) {

    getIndividualCounts(function (result){
        //you might want to do something is err is not null...
        /*  res.render('SQLtest', { 'title': 'SQL test',
         'result': result});*/
        console.log(result);
        res.status(200).send(JSON.stringify(result));
    });
});

/*function getCounts(callback) {

    connection.connect(function(err) {
        if (err){
            console.log(err)
        }

        var fileQuery='select count(file_id) from file';
        connection.query(fileQuery, function(error, result) {
            callback(error, result);
        });
    });
}*/

/*function getCounts(){
    connection.connect(function(err) {
        if (err) throw err

        var fileQuery='select count(file_id) from file';
        connection.query(fileQuery, function(err, result) {
            if (err){
                res.send(err);
            }
            else{
                setValue(result);
            }
        });

       var individialQuery='select count(ind_id) from individual';
       connection.query(individialQuery, function(err, result) {
           if (err){
               res.send(err);
           }
           else{console.log(result);
              setValue(result);

           }
       });
    });
}*/

app.get('/getFilesData', function(req, res) {
    getFiles(function (result){
        console.log(result);
        res.status(200).json(result);
    });
});
app.get('/getIndividualsData', function(req, res) {

    getIndividuals(function (result){
        console.log(result);
        res.status(200).send(result);
    });
});

/*function setValue(value) {
     dbStatus.push(value);
}*/

app.get('/clearData', function(req, res) {

    var clearFile='delete from file';
    connection.query(clearFile);
/*    connection.connect(function(err) {
        if (err) throw err




        connection.query(clearFile, function(err, result) {
            if (err){
                console.log(err);
                res.send(err);
            }
            else{
                console.log(result);
            }
        });

        var clearIndividual='delete from individual';

        connection.query(clearIndividual, function(err, result) {
            if (err){
                console.log(err);
                res.send(err);
            }
            else{
                console.log(result);
            }
        });
    });*/
    return res.status(200).send('All Tables are cleaned up.');
});

app.get('/getFileLog', function(req, res) {
        return res.status(200).send('No files were uploaded.');
});

app.get('/getGEDCOM', function(req, res) {
    return res.status(200).send('No files were uploaded.');

    // write method to get gedCOM data in c language and return data
});

app.get('/getGEDCOMFileName', function(req, res) {

    fs.readdir('uploads\\', function(err, filenames) {
        if (err) {
            console.log(err);
            return;
        }
        console.log(filenames);
        return res.status(200).send(JSON.stringify(filenames));
    });
});

function executeQuery(callback){
    var fileData = connection.query(sqlString);
    fileData.on('result', function(result) {
        callback(result);
    });
}

function getFiles(callback){

    connection.query('SELECT * FROM file', function(err, rows) {
        if (err) {
            console.log(err);
        }
        console.log(rows);
        callback(rows);
        // connected! (unless `err` is set)
    });
    /*   var fileQuery='select file_id,file_name,source,version,encoding,sub_name,sub_addr,num_individials,num_families from file';
     var fileData = connection.query(fileQuery);
     fileData.on('result', function(row) {
     callback(row);
     });*/
}

function getIndividuals(callback){

    connection.query('select * from individual', function(err, rows) {
        if (err) {
            console.log(err);
        }
        console.log(rows);
        callback(JSON.stringify(rows));
        // connected! (unless `err` is set)
    });
    /*    var individialQuery='select * from individual';
     var individualData=connection.query(individialQuery);
     individualData.on('result', function(row) {
     callback(row);
     });*/
}

function getFileCounts(callback){
    var fileQuery='select count(file_id) from file';
    var fileData = connection.query(fileQuery);
    fileData.on('result', function(row) {
        callback(row);
    });
}

function getIndividualCounts(callback){

    var individialQuery='select count(ind_id) from individual';
    var individualData=connection.query(individialQuery);
    individualData.on('result', function(row) {
        callback(row);
    });
}

/*
function readFiles(dirname, onFileContent, onError) {
    fs.readdir(dirname, function(err, filenames) {
        if (err) {
            onError(err);
            return;
        }
        return filenames;
        filenames.forEach(function(filename) {
            fs.readFile(dirname + filename, 'utf-8', function(err, content) {
                if (err) {
                    onError(err);
                    return;
                }
                onFileContent(filename, content);
            });
        });

    });
}*/


/*connection.connect(function(err) {
    if (err) throw err
    console.log('You are now connected...')

    connection.query('CREATE TABLE people(id int primary key, name varchar(255), age int, address text)', function(err, result) {
        if (err) throw err
        connection.query('INSERT INTO people (name, age, address) VALUES (?, ?, ?)', ['Larry', '41', 'California, USA'], function(err, result) {
            if (err) throw err
            connection.query('SELECT * FROM people', function(err, results) {
                if (err) throw err
                console.log(results[0].id)
                console.log(results[0].name)
                console.log(results[0].age)
                console.log(results[0].address)
            })
        })
    })
})*/

/*connection.connect(function(err) {
 if (err) throw err
 console.log('You are now connected...')

 connection.query('CREATE TABLE file(file_id  INT NOT NULL AUTO_INCREMENT, file_Name VARCHAR(60) NOT NULL, source varchar(250) NOT NULL, ' +
 'version  varchar(10) NOT NULL, encoding varchar(10) NOT NULL, sub_name varchar(62) NOT NULL, sub_addr varchar(256), num_individials INT, num_families INT, PRIMARY KEY(file_id) )', function(err, result) {
 if (err) throw err

 });

 connection.query('CREATE TABLE individual(ind_id  INT NOT NULL AUTO_INCREMENT, surname VARCHAR(256) NOT NULL, given_name varchar(256) NOT NULL, ' +
 'sex  varchar(1), fam_size INT, source_file INT, PRIMARY KEY(ind_id), ' +
 'CONSTRAINT FK_sourcefile FOREIGN KEY (source_file) REFERENCES file(file_id) ON DELETE CASCADE)', function(err, result) {
 if (err) throw err

 });
 });*/
