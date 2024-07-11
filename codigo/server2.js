const express = require('express');
const bodyParser = require('body-parser');
const { SerialPort } = require('serialport');
const { ReadlineParser } = require('@serialport/parser-readline');
const mysql = require('mysql');

const cors = require('cors');
const app = express();
app.use(cors());

const appPort = 3000; // Cambiado a appPort para evitar conflicto

// Configuración de la conexión MySQL
const connection = mysql.createConnection({
  host: 'localhost',
  user: 'root',
  password: '',
  port: 3307,
  multipleStatements: true // Permite ejecutar múltiples declaraciones SQL en una sola consulta
});

// Conectar y asegurar que la base de datos y la tabla existan
connection.connect(err => {
  if (err) {
    console.error('Error conectando a MySQL: ' + err.stack);
    return;
  }
  console.log('Conectado a MySQL como ID ' + connection.threadId);

  // Crear la base de datos y la tabla si no existen
  const createDbAndTableQuery = `
    CREATE DATABASE IF NOT EXISTS casa_inteligente;
    USE casa_inteligente;
    CREATE TABLE IF NOT EXISTS customer_logs (
      id INT AUTO_INCREMENT PRIMARY KEY,
      hora DATETIME NOT NULL,
      descripcion VARCHAR(255) NOT NULL
    );
  `;

  connection.query(createDbAndTableQuery, (error, results, fields) => {
    if (error) {
      console.error('Error al crear la base de datos o la tabla: ' + error.messae);
      return;
    }
    console.log('Base de datos y tabla aseguradas.');
  });
});


const portName = 'COM3';

const serialPort = new SerialPort({
  path: portName,
  baudRate: 9600
});

const parser = serialPort.pipe(new ReadlineParser({ delimiter: '\n' }));

parser.on('data', data => {
  const receivedData = data.trim(); // Convertir datos recibidos a string
  const currentTime = new Date().toISOString().slice(0, 19).replace('T', ' '); // Hora actual en formato SQL

  // Insertar datos en la base de datos
  const insertQuery = 'INSERT INTO customer_logs (hora, descripcion) VALUES (?, ?)';
  const values = [currentTime, receivedData];

  connection.query(insertQuery, values, (error, results, fields) => {
    if (error) {
      console.error('Error al insertar en la base de datos: ' + error.message);
      return;
    }
    console.log('Registro insertado correctamente.');
  });
});

app.use(bodyParser.urlencoded({ extended: true }));

app.post('/insertLog', (req, res) => {
  const { hora, descripcion } = req.body;

  // Insertar datos en la base de datos a través de la ruta POST
  const insertQuery = 'INSERT INTO customer_logs (hora, descripcion) VALUES (?, ?)';
  const values = [hora, descripcion];

  connection.query(insertQuery, values, (error, results, fields) => {
    if (error) {
      console.error('Error al insertar en la base de datos: ' + error.message);
      return;
    }
    console.log('Registro insertado correctamente.');
    res.send('Registro insertado correctamente.');
  });
});

// Ruta GET para obtener todos los registros de la tabla customer_logs
app.get('/getLogs', (req, res) => {
  const selectQuery = 'SELECT * FROM customer_logs';

  connection.query(selectQuery, (error, results, fields) => {
    if (error) {
      console.error('Error al obtener los registros de la base de datos: ' + error.message);
      res.status(500).send('Error al obtener los registros de la base de datos');
      return;
    }
    res.json(results);
  });
});

app.listen(appPort, () => {
  console.log(`Servidor Node.js escuchando en http://localhost:${appPort}`);
});
