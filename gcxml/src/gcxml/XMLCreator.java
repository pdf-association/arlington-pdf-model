/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package gcxml;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.FactoryConfigurationError;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.transform.OutputKeys;
import javax.xml.transform.Result;
import javax.xml.transform.Source;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;
import org.w3c.dom.Document;
import org.w3c.dom.Element;

/**
 *
 * @author fero
 */
public class XMLCreator {
    private String inputFolder;
    private String outputFolder;
    
    private String currentEntry;
    private int errorCount;
    
    private DocumentBuilderFactory domFactory = null;
    private DocumentBuilder domBuilder = null;
    private Document newDoc = null;

    public XMLCreator() {
        this.outputFolder = System.getProperty("user.dir") + "/xml/objects/";
        this.inputFolder = System.getProperty("user.dir") + "/csv/";
        this.currentEntry = "";
        this.errorCount = 0;
        try {
          domFactory = DocumentBuilderFactory.newInstance();
          domBuilder = domFactory.newDocumentBuilder();
          newDoc = domBuilder.newDocument();
        } catch (FactoryConfigurationError exp) {
          System.err.println(exp.toString());
        } catch (ParserConfigurationException exp) {
          System.err.println(exp.toString());
        } catch (Exception exp) {
          System.err.println(exp.toString());
        }
    }
    
    public void convertFile(String fileName, String delimiter) {
        inputFolder += fileName + ".csv"; 
        outputFolder += fileName + ".xml" ; 
        errorCount = 0;
        
        int rowsCount = -1;
        try {
          System.out.println("Processing " + fileName + ".csv ...");
          // Root element
          Element rootElement = newDoc.createElement("OBJECT");
          rootElement.setAttribute("id", fileName);
          //rootElement.setAttribute("type", "dictionary");
          newDoc.appendChild(rootElement);

          // Read csv file
          BufferedReader csvReader;
          csvReader = new BufferedReader(new FileReader(inputFolder));
          String[] colHeaders = null;

          String curLine = csvReader.readLine();
          if (curLine != null) {
            colHeaders = curLine.split(delimiter, -1);
            for (int i = 0; i < colHeaders.length; i++){
                colHeaders[i] = colHeaders[i].toUpperCase();
            }
          }
        // KEY | TYPE | SINCEVERSION | DEPRECATEDIN | REQUIRED | INDIRECTREFRENCE | REQUIRED VALUE | DEFAULT VALUE | POSSIBLE VALUES | SPECIALCASE | LINK
        // read entries
          while ((curLine = csvReader.readLine()) != null) {
              Element entryElement = newDoc.createElement("ENTRY");
              String[] colValues = curLine.split(delimiter,-1);
              currentEntry = colValues[0];
              //creates <NAME> node
              Element nameElement = nodeName(colValues[0]);
              // creates <VALUE> node
              // colValues[1] -> type
              // colValues[10] -> link(csv) VALIDATE(xml)
              // colValues[6], colValues[7], colValues[8] -> other values (optional)
              Element valuesEntry = nodeValues(colValues[1], colValues[10], colValues[6], colValues[7], colValues[8]);
              //creates <SINCEVERSION>, <DEPRECATEDIN>, <REQUIRED>, <INDIRECTREFRENCE>
              Element sinceversionElement = nodeSinceVersion(colValues[2]);
              Element deprecatedinElement = nodeDeprecatedIn(colValues[3]);
              Element requiredElement = nodeRequired(colValues[4]);
              Element indirectrefrenceElement = nodeIndirectRefrence(colValues[5]);
              if((nameElement != null) && (valuesEntry != null) && (sinceversionElement != null) &&
                      (deprecatedinElement != null) && (requiredElement != null) && (indirectrefrenceElement != null)){
                //append elements to entry
                entryElement.appendChild(nameElement);
                entryElement.appendChild(valuesEntry);
                entryElement.appendChild(requiredElement);
                entryElement.appendChild(indirectrefrenceElement);
                entryElement.appendChild(sinceversionElement);
                entryElement.appendChild(deprecatedinElement);
                //append entry to root
                rootElement.appendChild(entryElement);
              }
          }
          if(errorCount == 0){
            System.out.println("Finished succsefully.");
          }else{
              System.out.println("Processing failed! " +errorCount+" errors were encountered while processing object.");
          }
          csvReader.close();

          // Save the document to the disk file
          TransformerFactory tranFactory = TransformerFactory.newInstance();
          Transformer aTransformer = tranFactory.newTransformer();
          aTransformer.setOutputProperty(OutputKeys.INDENT, "yes");
          aTransformer.setOutputProperty(OutputKeys.METHOD, "xml");
          aTransformer.setOutputProperty("{http://xml.apache.org/xslt}indent-amount", "3");
          Source src = new DOMSource(newDoc);
          Result result = new StreamResult(new File(outputFolder));
          aTransformer.transform(src, result);


        } catch (IOException exp) {
          System.err.println(exp.toString());
        } catch (Exception exp) {
          System.err.println(exp.toString());
        }
    }

    private Element nodeName(String colValue) {
        Element tempElem = null;
        if(!colValue.isBlank()){
            tempElem = newDoc.createElement("NAME");
            tempElem.appendChild(newDoc.createTextNode(colValue));
        }else{
            System.out.println("\tERROR. While processing entry: " +currentEntry+ ". Failed to create NAME node. Missing value for key name.");
            ++errorCount;
        }
        return tempElem;
    }

    private Element nodeSinceVersion(String colValue) {
        Element tempElem = null;
        if(!colValue.isBlank()){
            tempElem = newDoc.createElement("SINCEVERSION");
            tempElem.appendChild(newDoc.createTextNode(colValue));
        }else{
            System.out.println("\tERROR. While processing entry: " +currentEntry+ ". Failed to create SINCEVERSION node. Missing value for since version.");
            ++errorCount;
        }
        return tempElem;
    }

    private Element nodeDeprecatedIn(String colValue) {
        Element tempElem = newDoc.createElement("DEPRECATEDIN");
        tempElem.appendChild(newDoc.createTextNode(colValue));
        return tempElem;
    }

    private Element nodeRequired(String colValue) {
        Element tempElem = null;
        if(!colValue.isBlank()){
            tempElem = newDoc.createElement("REQUIRED");
            tempElem.appendChild(newDoc.createTextNode(colValue.toLowerCase()));
        }else{
            System.out.println("\tERROR. While processing entry: " +currentEntry+ ". Failed to create REQUIRED node. Missing value for required. Shall be TRUE or FALSE.");
            ++errorCount;
        }
        return tempElem;
    }

    private Element nodeIndirectRefrence(String colValue) {
        Element tempElem = null;
        if(!colValue.isBlank()){
            tempElem = newDoc.createElement("INDIRECTREFRENCE");
            tempElem.appendChild(newDoc.createTextNode(colValue.toLowerCase()));
        }else{
            System.out.println("\tERROR. While processing entry: " +currentEntry+ ". Failed to create INDIRECTREFRENCE node. Missing value for indirect refrence. Shall be TRUE or FALSE.");
            ++errorCount;
        }
        return tempElem;
    }

    private Element nodeValues(String type, String validate, String requiredValue, String defaultValue, String possibleValues) {
        Element valuesElem = newDoc.createElement("VALUES");
        
        validate = validate.replace("[", "");
        validate = validate.replace("]", "");
        
        String[] types = null;
        String[] values = null;

        types = type.split(";", -1);
        values = validate.split(";", -1);
        
        
        if(types.length == values.length){
            for(int i = 0; i < types.length; i++){
                int k = 0;
                String[] temp = values[i].split(",",-1);
                for(int j = 0; j < temp.length; j++){
                    Element valueElem = newDoc.createElement("VALUE");
                    Element typeElem = newDoc.createElement("TYPE");
                    typeElem.appendChild(newDoc.createTextNode(types[i]));
                    valueElem.appendChild(typeElem);
                    if("dictionary".equals(types[i]) || "array".equals(types[i])){
                        Element validateElem = newDoc.createElement("VALIDATE");
                        String nodeVal = temp[j];
                        validateElem.appendChild(newDoc.createTextNode(nodeVal));
                        valueElem.appendChild(validateElem);
                        if(nodeVal.isBlank()){
                            System.out.println("\tWARNING. Missing value in entry: "+currentEntry+ ". VALIDATE node was created but has no value.");
                        }
                    }
                    if("name".equals(types[i]) && !requiredValue.isEmpty()){
                        Element shallbeElem = newDoc.createElement("SHALLBE");
                        shallbeElem.appendChild(newDoc.createTextNode(requiredValue));
                        valueElem.appendChild(shallbeElem);
                    }
                    valuesElem.appendChild(valueElem);
                    k++;
                }
            }
        }else{
            System.out.println("\tERROR. While processing entry: " +currentEntry+ ". Failed to create VALUES node. Types and links do not match.");
            ++errorCount;
        }
        return valuesElem;
    }
}
