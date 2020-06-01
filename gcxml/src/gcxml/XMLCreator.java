/*
 * XMLCreator.java
 * Copyright 2020 PDF Association, Inc. https://www.pdfa.org
 *
 * This material is based upon work supported by the Defense Advanced
 * Research Projects Agency (DARPA) under Contract No. HR001119C0079.
 * Any opinions, findings and conclusions or recommendations expressed
 * in this material are those of the author(s) and do not necessarily
 * reflect the views of the Defense Advanced Research Projects Agency
 * (DARPA). Approved for public release.
 *
 * SPDX-License-Identifier: Apache-2.0
 * Contributors: Roman Toda, Frantisek Forgac, Normex
 */
package gcxml;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
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
    
    private File[] list_of_files = null;
    private String delimiter = "";
    
    private String currentEntry;
    private int errorCount;
    
    private DocumentBuilderFactory domFactory = null;
    private DocumentBuilder domBuilder = null;
    private Document newDoc = null;

    public XMLCreator(File[] list_of_files, String delimiter) {
        this.outputFolder = System.getProperty("user.dir") + "/xml/objects/";
        this.inputFolder = System.getProperty("user.dir") + "/tsv/";
        
        // sort files by name alphabetically
        ArrayList<File> arrFile = new ArrayList<>();
        for(File file : list_of_files){
            arrFile.add(file);
        }
        arrFile.sort((p1, p2) -> p1.compareTo(p2));
        this.list_of_files = new File[arrFile.size()];
        for(int i=0; i < arrFile.size(); i++){
            this.list_of_files[i] = arrFile.get(i);
        }
        
        //this.list_of_files = list_of_files;
        this.delimiter = delimiter;
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
    
    public void convertFile() {
        //inputFolder += fileName + ".tsv"; 
        outputFolder += "_pdf_grammar" + ".xml" ; 
        errorCount = 0;
        
        int rowsCount = -1;
        int object_count = 0;
        try {
            // Root element
            Element rootElement = newDoc.createElement("PDF");
            rootElement.setAttribute("pdf_version", "2.0" );
            rootElement.setAttribute("grammar_version", "0.2.0" );
            rootElement.setAttribute("iso_ref", "ISO-32000" );
            newDoc.appendChild(rootElement);

            // Read tsv files
            for (File file : list_of_files) {
                object_count++;
                if (file.isFile() && file.canRead() && file.exists()) {
                    BufferedReader csvReader;
                    String path2 = inputFolder;
                    String fileName = file.getName().substring(0, file.getName().length()-4);
                    path2 += fileName + ".tsv";
                    csvReader = new BufferedReader(new FileReader(path2));
                    String[] colHeaders = null;
                    
                    // removes header row
                    String curLine = csvReader.readLine();
                    if (curLine != null) {
                      colHeaders = curLine.split(delimiter, -1);
                      for (int i = 0; i < colHeaders.length; i++){
                          colHeaders[i] = colHeaders[i].toUpperCase();
                      }
                    }
        // KEY | TYPE | SINCEVERSION | DEPRECATEDIN | REQUIRED | INDIRECTREFERENCE | REQUIRED VALUE | DEFAULT VALUE | POSSIBLE VALUES | SPECIALCASE | LINK
        
        // FUTURE WORK : add/change attributes
          Element objectElement = newDoc.createElement("OBJECT");
          objectElement.setAttribute("object_name", fileName);
          objectElement.setAttribute("object_number", String.format("%03d",object_count));
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
              Element valuesEntry = null;
              
              String types = colValues[1];
              String[] arrtypes = types.split(";", -1);  
              boolean isLinkable = false;
              for(int i = 0; i < arrtypes.length; i++){
                if("dictionary".equals(arrtypes[i]) || "array".equals(arrtypes[i]) ||"stream".equals(arrtypes[i])){
                    isLinkable = true;
                }
              }
              
              if(isLinkable == true){
                  valuesEntry = nodeValuesLinkable(colValues[1], colValues[10]);
              }else{
                  valuesEntry = nodeValues(colValues[1], colValues[6], colValues[7], colValues[8]);
              }
              //creates <SINCEVERSION>, <DEPRECATEDIN>, <REQUIRED>, <INDIRECTREFERENCE>
              Element sinceversionElement = nodeSinceVersion(colValues[2]);
              Element deprecatedinElement = nodeDeprecatedIn(colValues[3]);
              Element requiredElement = nodeRequired(colValues[4]);
              Element indirectreferenceElement = nodeIndirectReference(colValues[5]);
              if((nameElement != null) && (valuesEntry != null) && (sinceversionElement != null) &&
                      (deprecatedinElement != null) && (requiredElement != null) && (indirectreferenceElement != null)){
                //append elements to entry
                entryElement.appendChild(nameElement);
                entryElement.appendChild(valuesEntry);
                entryElement.appendChild(requiredElement);
                entryElement.appendChild(indirectreferenceElement);
                entryElement.appendChild(sinceversionElement);
                entryElement.appendChild(deprecatedinElement);
                
                objectElement.appendChild(entryElement);
                //append entry to root
                rootElement.appendChild(objectElement);
              }
          }
          if(errorCount == 0){
            System.out.println("Finished succsefully.");
          }else{
              System.out.println("Processing failed! " +errorCount+" errors were encountered while processing object.");
          }
          csvReader.close();
          
                }
          }
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

    private Element nodeIndirectReference(String colValue) {
        Element tempElem = null;
        if(!colValue.isBlank()){
            tempElem = newDoc.createElement("INDIRECTREFERENCE");
            tempElem.appendChild(newDoc.createTextNode(colValue.toLowerCase()));
        }else{
            System.out.println("\tERROR. While processing entry: " +currentEntry+ ". Failed to create INDIRECTREFERENCE node. Missing value for indirect reference. Shall be TRUE or FALSE.");
            ++errorCount;
        }
        return tempElem;
    }

    private Element nodeValuesLinkable(String type, String validate) {        
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
            //Element typeElem = newDoc.createElement("TYPE");
            //typeElem.appendChild(newDoc.createTextNode(types[i]));
            valueElem.setAttribute("type", types[i]);
            if("dictionary".equals(types[i]) || "array".equals(types[i]) || "stream".equals(types[i])){
                Element validateElem = newDoc.createElement("VALIDATE");
                String nodeVal = temp[j];
                valueElem.appendChild(newDoc.createTextNode(nodeVal));
                //valueElem.appendChild(validateElem);
                if(nodeVal.isBlank()){
                    System.out.println("\tWARNING. Missing value in entry: "+currentEntry+ ". VALIDATE node was created but has no value.");
                }
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

    private Element nodeValues(String type, String requiredValue, String defaultValue, String possibleValues) {
        Element valuesElem = newDoc.createElement("VALUES");
        
        String[] types = null;
        String[] posValues = null;
        
        possibleValues = possibleValues.replace("[", "");
        possibleValues = possibleValues.replace("]", "");
        
        types = type.split(";", -1);
        //posValues = possibleValues.split(";", -1);
        
        if(!possibleValues.isBlank()){
        posValues = possibleValues.split(";", -1);
        for(int i = 0; i < types.length; i++){
            int k = 0;
            String[] temp = posValues[i].split(",",-1);
            for(int j = 0; j < temp.length; j++){
                Element valueElem = newDoc.createElement("VALUE");
            //Element typeElem = newDoc.createElement("TYPE");
            //typeElem.appendChild(newDoc.createTextNode(types[i]));
            valueElem.setAttribute("type", types[i]);
                if(!temp[j].isEmpty()){
                    // SHALLBE -  rename to something more meaningful, eg. ACTUALVALUE
                    //Element shallbeElem = newDoc.createElement("SHALLBE");
                valueElem.appendChild(newDoc.createTextNode(temp[j]));
                    //valueElem.appendChild(shallbeElem);
                }
                valuesElem.appendChild(valueElem);
            }
            }
        }else{
            for(int i = 0; i < types.length; i++){
            int k = 0;
                Element valueElem = newDoc.createElement("VALUE");
            //Element typeElem = newDoc.createElement("TYPE");
            //typeElem.appendChild(newDoc.createTextNode(types[i]));
            valueElem.setAttribute("type", types[i]);
                valuesElem.appendChild(valueElem);
            }
        }
         
        return valuesElem;
    }
}
