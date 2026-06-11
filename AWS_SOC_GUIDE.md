# Guía del Laboratorio Académico: Plataforma SOC Orientada a AWS

Esta guía detalla la arquitectura, el despliegue y los pasos para realizar la simulación del incidente tipo **Capital One** en un entorno controlado utilizando la plataforma SOC.

---

## 1. Diagrama de la Arquitectura

El flujo de telemetría de seguridad y respuesta ante incidentes funciona de la siguiente manera:

```text
  [ Eventos en AWS ]
       │
       ▼ (IAM, S3, EC2, STS, VPC)
  [ AWS CloudTrail ] (Activa Registro de Eventos de Administración y Datos)
       │
       ▼ (Publicación automática en bus default)
  [ Amazon EventBridge ] (Regla filtra eventos de alto riesgo)
       │
       ▼ (Dispara)
  [ Función Lambda (Python) ] (Normaliza JSON, calcula HMAC-SHA256 con webhook secret)
       │
       ▼ POST /api/v1/aws/events (con firma en cabecera X-SOC-Signature)
  [ Backend C++ (Drogon) ]
       │
       ├──► 1. Valida firma HMAC-SHA256 (seguridad de origen)
       ├──► 2. Ingesta el evento en la tabla `aws_events` de SQLite
       ├──► 3. Motor de Correlación: Compara contra reglas activas
       │         │
       │         └──► Si hay coincidencia: Genera Incidente (Ticket) Automáticamente
       │              ├──► Crea evidencia y timeline forense
       │              └──► Genera notificaciones internas y logs de alerta
       │
       ▼ (Sincronización en tiempo real mediante API Polling)
  [ Panel Dashboard SOC ] (Analista visualiza alertas, línea de tiempo e investigación)
```

---

## 2. Configuración en AWS (Vía Terraform)

La infraestructura necesaria para recolectar, procesar y transmitir los eventos está completamente automatizada mediante los archivos de Terraform en el directorio [terraform](file:///c:/Users/sgsal/OneDrive/Escritorio/Cibersegurida%20intento/terraform/):

1. **CloudTrail ([cloudtrail.tf](file:///c:/Users/sgsal/OneDrive/Escritorio/Cibersegurida%20intento/terraform/cloudtrail.tf))**:
   - Crea un trail multirregión con entrega a un bucket S3 específico.
   - Configura selectores de eventos para auditar llamadas de API de administración y de datos de S3 (objeto `GetObject`).
2. **EventBridge ([eventbridge.tf](file:///c:/Users/sgsal/OneDrive/Escritorio/Cibersegurida%20intento/terraform/eventbridge.tf))**:
   - Declara una regla que intercepta eventos provenientes de `aws.iam`, `aws.s3`, `aws.ec2`, `aws.sts` y `aws.vpc`.
   - Establece como destino la función Lambda de procesamiento.
3. **Lambda ([lambda.tf](file:///c:/Users/sgsal/OneDrive/Escritorio/Cibersegurida%20intento/terraform/lambda.tf))**:
   - Empaqueta el archivo [lambda_function.py](file:///c:/Users/sgsal/OneDrive/Escritorio/Cibersegurida%20intento/lambda/lambda_function.py).
   - Configura las variables de entorno `SOC_BACKEND_URL` (apuntando al balanceador de carga del backend) y `WEBHOOK_SECRET` para firmar digitalmente las peticiones.
   - Otorga permisos a EventBridge para invocar la función.

---

## 3. Instrucciones de Despliegue

### Despliegue Local con Docker (Recomendado para pruebas y desarrollo)

Para iniciar el backend y el panel de visualización en su máquina local:

1. **Clonar y compilar el contenedor**:
   Ejecute el siguiente comando en la raíz del proyecto para descargar dependencias, compilar el servidor C++ Drogon y levantar el servicio en el puerto `8080`:
   ```bash
   docker compose up --build
   ```

2. **Abrir el Panel SOC**:
   Abra el archivo [index.html](file:///c:/Users/sgsal/OneDrive/Escritorio/Cibersegurida%20intento/frontend/index.html) directamente en cualquier navegador web moderno.
   El panel se conectará automáticamente al backend local en `http://127.0.0.1:8080`.

3. **Iniciar Sesión como Administrador**:
   - Haga clic en **ADMIN INICIAR** en la esquina superior derecha.
   - Ingrese las credenciales predeterminadas:
     - **Usuario**: `admin`
     - **Contraseña**: `admin-password`
   - Esto habilitará la pestaña **Reglas** y le permitirá ver la pestaña de **Ingesta AWS**.

---

## 4. Pruebas y Simulación del Escenario Académico (Capital One)

El laboratorio académico simula un compromiso en la nube idéntico al ataque sufrido por Capital One (abuso de SSRF y robo de credenciales de metadatos EC2).

### Paso a Paso de la Simulación

1. Vaya a la pestaña **Simulador** en el panel web.
2. Haga clic en el botón **"Simular Capital One"**.
3. El simulador enviará de forma consecutiva la cadena de eventos a la API del backend:
   - **Paso 1 (Modificación de SG)**: Abre el puerto 80/tráfico HTTP global.
   - **Paso 2 (Compromiso SSRF)**: STS emite credenciales temporales para la instancia EC2 (`EC2WafInstanceRole`) pero el atacante las utiliza desde una dirección IP pública externa (`203.0.113.50`). El motor detecta la anomalía de IP y lanza una alerta crítica: **"SSRF contra EC2 Metadata Service"**.
   - **Paso 3 (Enumeración)**: El atacante ejecuta `GetCallerIdentity` y `ListBuckets` para mapear los recursos de almacenamiento S3. El backend correlaciona esta secuencia y genera la alerta: **"Enumeración de recursos AWS"**.
   - **Paso 4 (Exfiltración)**: El atacante accede a archivos en el bucket sensible `capital-one-sensitive-data` y descarga múltiples archivos en un rango de 10 segundos. El motor de correlación lo detecta y crea de forma automática el incidente: **"Acceso a buckets S3 sensibles"** y **"Descarga masiva de archivos S3"** (Severidad: Crítica).
4. **Verificación Forense**:
   - Vuelva a la pestaña **Incidentes**. Verá que se han creado los tickets de severidad **Alta** y **Crítica** automáticamente sin intervención humana.
   - Haga clic en cualquiera de los incidentes.
   - Revise el bloque de **Evidencia Forense** para examinar qué eventos de CloudTrail dispararon la alarma.
   - Haga clic sobre el enlace de evidencia para abrir el visor de JSON y analizar el objeto de CloudTrail crudo.
   - Analice la **Línea Temporal** que muestra los hitos del incidente con precisión de milisegundos.
5. **Limpieza del Entorno**:
   - Si desea reiniciar la simulación local de cero y limpiar la base de datos SQLite, detenga los contenedores y destruya su volumen persistente:
     ```bash
     docker compose down --volumes
     docker compose up
     ```
